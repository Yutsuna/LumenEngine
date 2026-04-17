/**
 * @file SpatialRegistry.cpp
 * @brief Implementation of the FSpatialRegistry class.
 */

#include "World/SpatialRegistry.hpp"

/**
 * FDeltaTracker
 */

void LumenEngine::Engine::FSpatialRegistry::FDeltaTracker::Resize ( const USize InNumBits )
{
    for ( FAtomicBitset &Bitset : ComponentDirtyStates )
    {
        Bitset.Resize( InNumBits );
    }
}

void LumenEngine::Engine::FSpatialRegistry::FDeltaTracker::ClearAll () noexcept
{
    for ( FAtomicBitset &Bitset : ComponentDirtyStates )
    {
        Bitset.ClearAll();
    }
}

/**
 * FSpatialRegistry
 */

LumenEngine::Engine::FSpatialRegistry &LumenEngine::Engine::FSpatialRegistry::Get () noexcept
{
    static FSpatialRegistry Instance;

    return Instance;
}

void LumenEngine::Engine::FSpatialRegistry::RegisterSpatialEntity ( ActorID InId )
{
    TSharedUniqueLock<FSharedMutex> Lock( RegistryMutex );

    if ( WorkingData.IDToIndex.contains( InId ) )
    {
        return;
    }

    const USize NewIndex = WorkingData.EntityIDs.size();

    WorkingData.EntityIDs.emplace_back( InId );
    WorkingData.Transforms.emplace_back( Maths::FMatrix4x4f::Identity() );
    WorkingData.Meshes.emplace_back( RHI::FMeshHandle::InvalidID );
    WorkingData.Shaders.emplace_back( RHI::FPipelineHandle::InvalidID );
    WorkingData.IDToIndex.emplace( InId, NewIndex );

    /** INFO: Resize bitsets and flag structural sync for all 3 buffers */
    for ( USize Index = 0U; Index < 3U; ++Index )
    {
        DeltaTrackers[Index].bMetadataDirty = true;
        DeltaTrackers[Index].Resize( WorkingData.EntityIDs.size() );
    }
}

void LumenEngine::Engine::FSpatialRegistry::UnregisterSpatialEntity ( ActorID InId )
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
        WorkingData.Meshes[IndexToRemove]     = WorkingData.Meshes[LastIndex];
        WorkingData.Shaders[IndexToRemove]    = WorkingData.Shaders[LastIndex];

        WorkingData.IDToIndex[LastActorID] = IndexToRemove;
    }

    WorkingData.EntityIDs.pop_back();
    WorkingData.Transforms.pop_back();
    WorkingData.Meshes.pop_back();
    WorkingData.Shaders.pop_back();
    WorkingData.IDToIndex.erase( InId );

    for ( USize Index = 0U; Index < 3U; ++Index )
    {
        DeltaTrackers[Index].bMetadataDirty = true;
        DeltaTrackers[Index].Resize( WorkingData.EntityIDs.size() );
    }
}

void LumenEngine::Engine::FSpatialRegistry::UpdateTransform ( ActorID InId, const Maths::FMatrix4x4f &InTransform ) noexcept
{
    TSharedLock<FSharedMutex> Lock( RegistryMutex );
    const FSpatialRegistryData::FMapIterator It = WorkingData.IDToIndex.find( InId );

    if ( It != WorkingData.IDToIndex.end() )
    {
        const USize Index             = It->second;
        WorkingData.Transforms[Index] = InTransform;
        MarkComponentDirty( Index, ESpatialComponentType::Transform );
    }
}

void LumenEngine::Engine::FSpatialRegistry::AssignRenderData ( ActorID InId, RHI::FMeshHandle InMesh, RHI::FPipelineHandle InShader ) noexcept
{
    TSharedLock<FSharedMutex> Lock( RegistryMutex );
    const FSpatialRegistryData::FMapIterator It = WorkingData.IDToIndex.find( InId );

    if ( It != WorkingData.IDToIndex.end() )
    {
        const USize Index          = It->second;
        WorkingData.Meshes[Index]  = InMesh;
        WorkingData.Shaders[Index] = InShader;
        MarkComponentDirty( Index, ESpatialComponentType::Mesh );
        MarkComponentDirty( Index, ESpatialComponentType::Shader );
    }
}

void LumenEngine::Engine::FSpatialRegistry::MarkComponentDirty ( USize InIndex, ESpatialComponentType InComponent ) noexcept
{
    const USize ComponentIdx = static_cast<USize>( InComponent );
    for ( USize TrackerIdx = 0U; TrackerIdx < 3U; ++TrackerIdx )
    {
        DeltaTrackers[TrackerIdx].ComponentDirtyStates[ComponentIdx].Set( InIndex );
    }
}

void LumenEngine::Engine::FSpatialRegistry::Publish () noexcept
{
    TSharedUniqueLock<FSharedMutex> Lock( RegistryMutex );

    SnapshotBuffer.WriteBuffer(
        [this] ( FSpatialRegistryData &OutData )
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
                [&] ( USize Index ) { OutData.Transforms[Index] = WorkingData.Transforms[Index]; } );

            Tracker.ComponentDirtyStates[static_cast<USize>( ESpatialComponentType::Mesh )].ForEachSetBitAndClear(
                [&] ( USize Index ) { OutData.Meshes[Index] = WorkingData.Meshes[Index]; } );

            Tracker.ComponentDirtyStates[static_cast<USize>( ESpatialComponentType::Shader )].ForEachSetBitAndClear(
                [&] ( USize Index ) { OutData.Shaders[Index] = WorkingData.Shaders[Index]; } );
        } );
}

void LumenEngine::Engine::FSpatialRegistry::SwapReadBuffers () noexcept
{
    SnapshotBuffer.SwapReadBuffers();
}

const LumenEngine::Engine::FSpatialRegistryData &LumenEngine::Engine::FSpatialRegistry::GetReadSnapshot () const noexcept
{
    return SnapshotBuffer.ReadBuffer();
}