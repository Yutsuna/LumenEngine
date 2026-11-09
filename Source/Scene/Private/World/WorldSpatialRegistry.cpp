/**
 * @file SpatialRegistry.cpp
 * @brief Implementation of the FSpatialRegistry class.
 */

#include <World/WorldSpatialRegistry.hpp>

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

Lumen::FSpatialRegistry &Lumen::FSpatialRegistry::Get () noexcept
{
    static FSpatialRegistry Instance;

    return Instance;
}

void Lumen::FSpatialRegistry::RegisterSpatialEntity ( ActorID InId )
{
    TSharedUniqueLock<FSharedMutex> Lock( RegistryMutex );

    if ( WorkingData.IDToIndex.contains( InId ) )
    {
        return;
    }

    const USize NewIndex = WorkingData.EntityIDs.size();

    WorkingData.EntityIDs.emplace_back( InId );
    WorkingData.Transforms.emplace_back( Maths::FMatrix4x4f::Identity() );
    WorkingData.IDToIndex.emplace( InId, NewIndex );

    ResizeTrackersAndFlagStructuralChange();
}

void Lumen::FSpatialRegistry::UnregisterSpatialEntity ( ActorID InId )
{
    TSharedUniqueLock<FSharedMutex> Lock( RegistryMutex );
    const FSpatialRegistryData::FMapIterator It = WorkingData.IDToIndex.find( InId );

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

void Lumen::FSpatialRegistry::UpdateTransform ( ActorID InId, const Maths::FMatrix4x4f &InTransform ) noexcept
{
    TSharedUniqueLock<FSharedMutex> Lock( RegistryMutex );
    const FSpatialRegistryData::FMapIterator It = WorkingData.IDToIndex.find( InId );

    if ( It != WorkingData.IDToIndex.end() )
    {
        const USize Index             = It->second;
        WorkingData.Transforms[Index] = InTransform;
        MarkComponentDirty( Index, ESpatialComponentType::Transform );
    }
}

void Lumen::FSpatialRegistry::Publish () noexcept
{
    TSharedUniqueLock<FSharedMutex> Lock( RegistryMutex );

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

    SnapshotBuffer.WriteBuffer( [this] ( FSpatialRegistryData &OutData ) noexcept -> void
    {
        /** INFO: Identify which of the 3 buffers we are currently writing to */
        const USize BufferIdx  = static_cast<USize>( &OutData - SnapshotBuffer.GetBuffers() );
        FDeltaTracker &Tracker = DeltaTrackers[BufferIdx];

        if ( Tracker.bMetadataDirty )
        {
            OutData                = WorkingData;
            Tracker.bMetadataDirty = false;
            Tracker.ClearAll();
            return;
        }

        /** INFO: Granular Sync: Only copy what changed for THIS buffer instance */
        Tracker.ComponentDirtyStates[static_cast<USize>( ESpatialComponentType::Transform )].ForEachSetBitAndClear(
            [&] ( const USize Index ) noexcept -> void
        {
            OutData.Transforms[Index] = WorkingData.Transforms[Index];
        } );
    } );
}

void Lumen::FSpatialRegistry::SwapReadBuffers () noexcept
{
    SnapshotBuffer.SwapReadBuffers();
}

const Lumen::FSpatialRegistryData &Lumen::FSpatialRegistry::GetReadSnapshot () const noexcept
{
    return SnapshotBuffer.ReadBuffer();
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

    /** INFO: every buffer needs a full copy on the next Publish */
    for ( USize BufferIdx = 0U; BufferIdx < 3U; ++BufferIdx )
    {
        DeltaTrackers[BufferIdx].bMetadataDirty = true;
        DeltaTrackers[BufferIdx].Resize( EntityCount );
    }

    /** INFO: Pending must cover the new entity range for future updates. */
    for ( USize CompIdx = 0U; CompIdx < Thread::TripleBufferCount; ++CompIdx )
    {
        Pending[CompIdx].Resize( EntityCount );
    }
}
