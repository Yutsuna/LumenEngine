/**
 * @file WorldSpatialRegistry.cpp
 * @brief Implementation of the FSpatialRegistry class.
 */

#include <World/WorldSpatialRegistry.hpp>

#include <World/WorldTypes.hpp>

#include <Templates/Unexpected.hpp>

/**
 * FSpatialRegistryStorage
 */

void Lumen::FSpatialRegistryStorage::Reserve ( const FSpatialRegistryCapacities &InCapacities ) noexcept
{
    EntityIDs.reserve( InCapacities.EntityCapacity );
    Transforms.reserve( InCapacities.EntityCapacity );
}

void Lumen::FSpatialRegistryStorage::Reset () noexcept
{
    EntityIDs.clear();
    Transforms.clear();
}

Lumen::FSpatialSnapshotView Lumen::FSpatialRegistryStorage::GetView () const noexcept
{
    return FSpatialSnapshotView{ .EntityIDs  = TSpan<const ActorID>{ EntityIDs.data(), EntityIDs.size() },
                                 .Transforms = TSpan<const Maths::FMat4x4f>{ Transforms.data(), Transforms.size() } };
}

/**
 * FDeltaTracker
 */

void Lumen::FSpatialRegistry::FDeltaTracker::Resize ( const USize InNumBits )
{
    for ( FAtomicBitset &Bitset : ComponentDirtyStates )
    {
        Bitset.Resize( InNumBits );
    }
}

void Lumen::FSpatialRegistry::FDeltaTracker::ClearAll () noexcept
{
    for ( FAtomicBitset &Bitset : ComponentDirtyStates )
    {
        Bitset.ClearAll();
    }
}

/**
 * FSpatialRegistry
 */

/**
 * Public
 */

Lumen::FSpatialRegistry::FSpatialRegistry () noexcept
    : FSpatialRegistry( FSpatialRegistryCapacities{ .EntityCapacity = FWorldDesc{}.MaxActors } )
{
    /** Empty */
}

Lumen::FSpatialRegistry::FSpatialRegistry ( const FSpatialRegistryCapacities &InCapacities ) noexcept
    : Capacities( InCapacities ), Slots{}, SnapshotBuffer( Slots )
{
    for ( FSpatialRegistryStorage &Slot : Slots )
    {
        Slot.Reserve( Capacities );
    }

    WorkingData.EntityIDs.reserve( Capacities.EntityCapacity );
    WorkingData.Transforms.reserve( Capacities.EntityCapacity );
    WorkingData.IDToIndex.reserve( Capacities.EntityCapacity );

    /** INFO: The array-backed triple buffer starts dirty; all three empty slots are equivalent and fully reserved. */
    SnapshotBuffer.SwapReadBuffers();
}

Lumen::FSpatialRegistry &Lumen::FSpatialRegistry::Get () noexcept
{
    static FSpatialRegistry Instance;

    return Instance;
}

void Lumen::FSpatialRegistry::RegisterSpatialEntity ( const ActorID InId ) noexcept
{
    static_cast<void>( TryRegisterSpatialEntity( InId ) );
}

Lumen::TExpected<void, Lumen::ESpatialRegistryError>
Lumen::FSpatialRegistry::TryRegisterSpatialEntity ( const ActorID InId ) noexcept
{
    HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );

    if ( WorkingData.IDToIndex.contains( InId ) )
    {
        return {};
    }

    if ( WorkingData.EntityIDs.size() >= Capacities.EntityCapacity ) [[unlikely]]
    {
        DroppedRegistrationCount.fetch_add( 1ULL, std::memory_order_relaxed );
        return MakeUnexpected( ESpatialRegistryError::CapacityExceeded );
    }

    const USize NewIndex = WorkingData.EntityIDs.size();

    WorkingData.EntityIDs.emplace_back( InId );
    WorkingData.Transforms.emplace_back( Maths::FMat4x4f::Identity() );
    WorkingData.IDToIndex.emplace( InId, NewIndex );

    ResizeTrackersAndFlagStructuralChange();
    return {};
}

void Lumen::FSpatialRegistry::UnregisterSpatialEntity ( const ActorID InId ) noexcept
{
    HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );
    const FWorkingData::FMapIterator It = WorkingData.IDToIndex.find( InId );

    if ( It == WorkingData.IDToIndex.end() )
    {
        return;
    }

    const USize IndexToRemove = It->second;
    const USize LastIndex     = WorkingData.EntityIDs.size() - 1U;

    if ( IndexToRemove != LastIndex )
    {
        const ActorID LastActorID = WorkingData.EntityIDs[LastIndex];

        WorkingData.EntityIDs[IndexToRemove]  = WorkingData.EntityIDs[LastIndex];
        WorkingData.Transforms[IndexToRemove] = WorkingData.Transforms[LastIndex];
        WorkingData.IDToIndex[LastActorID]    = IndexToRemove;
    }

    WorkingData.EntityIDs.pop_back();
    WorkingData.Transforms.pop_back();
    WorkingData.IDToIndex.erase( InId );

    ResizeTrackersAndFlagStructuralChange();
}

void Lumen::FSpatialRegistry::UpdateTransform ( const ActorID InId, const Maths::FMat4x4f &InTransform ) noexcept
{
    HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );
    const FWorkingData::FMapIterator It = WorkingData.IDToIndex.find( InId );

    if ( It != WorkingData.IDToIndex.end() )
    {
        const USize Index             = It->second;
        WorkingData.Transforms[Index] = InTransform;
        MarkComponentDirty( Index, ESpatialComponentType::Transform );
    }
}

void Lumen::FSpatialRegistry::Publish () noexcept
{
    HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );

    /**
     * INFO: Drain Pending into every buffer's tracker BEFORE writing to the
     *       current buffer. We hold the exclusive lock, so no concurrent
     *       UpdateTransform is active — the drain is race-free even though
     *       the bitsets are atomic.
     */
    for ( USize Comp = 0U; Comp < static_cast<USize>( ESpatialComponentType::Count ); ++Comp )
    {
        Pending[Comp].ForEachSetBitAndClear( [this, Comp] ( const USize Index ) noexcept -> void
        {
            for ( USize BufferIdx = 0U; BufferIdx < Thread::TripleBufferCount; ++BufferIdx )
            {
                DeltaTrackers[BufferIdx].ComponentDirtyStates[Comp].Set( Index );
            }
        } );
    }

    SnapshotBuffer.WriteBuffer( [this] ( FSpatialRegistryStorage &OutStorage ) noexcept -> void
    {
        /** INFO: Identify which of the 3 slots we are currently writing to */
        const USize BufferIdx  = static_cast<USize>( &OutStorage - SnapshotBuffer.GetBuffers() );
        FDeltaTracker &Tracker = DeltaTrackers[BufferIdx];

        if ( Tracker.bMetadataDirty )
        {
            /** INFO: Every slot was reserved at construction, so these appends never allocate. */
            OutStorage.Reset();
            for ( const ActorID EntityId : WorkingData.EntityIDs )
            {
                OutStorage.EntityIDs.emplace_back( EntityId );
            }
            for ( const Maths::FMat4x4f &Transform : WorkingData.Transforms )
            {
                OutStorage.Transforms.emplace_back( Transform );
            }

            Tracker.bMetadataDirty = false;
            Tracker.ClearAll();
            return;
        }

        /** INFO: Granular Sync: Only copy what changed for THIS slot instance */
        Tracker.ComponentDirtyStates[static_cast<USize>( ESpatialComponentType::Transform )].ForEachSetBitAndClear(
            [&] ( const USize Index ) noexcept -> void
        {
            OutStorage.Transforms[Index] = WorkingData.Transforms[Index];
        } );
    } );

    LastPublishedSequence.fetch_add( 1ULL, std::memory_order_release );
}

void Lumen::FSpatialRegistry::SwapReadBuffers () noexcept
{
    if ( not SnapshotBuffer.IsDirty() )
    {
        return;
    }

    SnapshotBuffer.SwapReadBuffers();

    const UInt64 Published = LastPublishedSequence.load( std::memory_order_acquire );
    const UInt64 Consumed  = LastConsumedSequence.load( std::memory_order_relaxed );

    if ( Published > Consumed )
    {
        if ( Published - Consumed > 1ULL )
        {
            DroppedSnapshotCount.fetch_add( Published - Consumed - 1ULL, std::memory_order_relaxed );
        }
        LastConsumedSequence.store( Published, std::memory_order_release );
    }
}

Lumen::FSpatialSnapshotView Lumen::FSpatialRegistry::GetReadSnapshot () const noexcept
{
    return SnapshotBuffer.ReadBuffer().GetView();
}

Lumen::FSpatialRegistryCapacities Lumen::FSpatialRegistry::GetCapacities () const noexcept
{
    return Capacities;
}

Lumen::UInt64 Lumen::FSpatialRegistry::GetLastPublishedSequence () const noexcept
{
    return LastPublishedSequence.load( std::memory_order_acquire );
}

Lumen::UInt64 Lumen::FSpatialRegistry::GetLastConsumedSequence () const noexcept
{
    return LastConsumedSequence.load( std::memory_order_acquire );
}

Lumen::UInt64 Lumen::FSpatialRegistry::GetDroppedSnapshotCount () const noexcept
{
    return DroppedSnapshotCount.load( std::memory_order_relaxed );
}

Lumen::UInt64 Lumen::FSpatialRegistry::GetDroppedRegistrationCount () const noexcept
{
    return DroppedRegistrationCount.load( std::memory_order_relaxed );
}

/**
 * Private
 */

void Lumen::FSpatialRegistry::MarkComponentDirty ( const USize InIndex, const ESpatialComponentType InComponent ) noexcept
{
    Pending[static_cast<USize>( InComponent )].Set( InIndex );
}

void Lumen::FSpatialRegistry::ResizeTrackersAndFlagStructuralChange () noexcept
{
    const USize EntityCount = WorkingData.EntityIDs.size();

    /** INFO: every slot needs a full copy on the next Publish */
    for ( USize BufferIdx = 0U; BufferIdx < Thread::TripleBufferCount; ++BufferIdx )
    {
        DeltaTrackers[BufferIdx].bMetadataDirty = true;
        DeltaTrackers[BufferIdx].Resize( EntityCount );
    }

    /** INFO: Pending must cover the new entity range for future updates. */
    for ( USize CompIdx = 0U; CompIdx < static_cast<USize>( ESpatialComponentType::Count ); ++CompIdx )
    {
        Pending[CompIdx].Resize( EntityCount );
    }
}
