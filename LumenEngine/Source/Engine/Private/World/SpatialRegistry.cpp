/**
 * @file SpatialRegistry.cpp
 * @brief Implementation of the FSpatialRegistry class, which manages spatial and renderable data for actors in the engine.
 */

#include "World/SpatialRegistry.hpp"

LumenEngine::Engine::FSpatialRegistry &LumenEngine::Engine::FSpatialRegistry::Get () noexcept
{
    static FSpatialRegistry Instance;

    return Instance;
}

void LumenEngine::Engine::FSpatialRegistry::RegisterSpatialEntity ( ActorID InId )
{
    TLockGuard<FMutex> Lock( RegistryMutex );

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

    for ( USize Index = 0U; Index < 3U; ++Index )
    {
        DeltaTrackers[Index].bMetadataDirty = true;
        DeltaTrackers[Index].DirtyIndices.clear();
        DeltaTrackers[Index].IsDirty.assign( WorkingData.EntityIDs.size(), false );
    }
}

void LumenEngine::Engine::FSpatialRegistry::UnregisterSpatialEntity ( ActorID InId )
{
    TLockGuard<FMutex> Lock( RegistryMutex );
    const auto It = WorkingData.IDToIndex.find( InId );

    if ( It == WorkingData.IDToIndex.end() )
    {
        return;
    }

    const USize IndexToRemove = It->second;
    const USize LastIndex     = WorkingData.EntityIDs.size() - 1U;

    /** INFO: To maintain packed data without gaps, we move the last element to the removed index and update the mapping. */
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
        DeltaTrackers[Index].DirtyIndices.clear();
        DeltaTrackers[Index].IsDirty.assign( WorkingData.EntityIDs.size(), false );
    }
}

void LumenEngine::Engine::FSpatialRegistry::UpdateTransform ( ActorID InId, const Maths::FMatrix4x4f &InTransform ) noexcept
{
    TLockGuard<FMutex> Lock( RegistryMutex );
    const auto It = WorkingData.IDToIndex.find( InId );

    if ( It == WorkingData.IDToIndex.end() )
    {
        return;
    }

    const USize Index             = It->second;
    WorkingData.Transforms[Index] = InTransform;

    for ( USize TrackerIdx = 0U; TrackerIdx < 3U; ++TrackerIdx )
    {
        if ( not DeltaTrackers[TrackerIdx].IsDirty[Index] )
        {
            DeltaTrackers[TrackerIdx].IsDirty[Index] = true;
            DeltaTrackers[TrackerIdx].DirtyIndices.push_back( Index );
        }
    }
}

void LumenEngine::Engine::FSpatialRegistry::AssignRenderData ( ActorID InId, RHI::FMeshHandle InMesh, RHI::FPipelineHandle InShader ) noexcept
{
    TLockGuard<FMutex> Lock( RegistryMutex );
    const auto It = WorkingData.IDToIndex.find( InId );

    if ( It == WorkingData.IDToIndex.end() )
    {
        return;
    }
    WorkingData.Meshes[It->second]  = InMesh;
    WorkingData.Shaders[It->second] = InShader;

    for ( USize Index = 0U; Index < 3U; ++Index )
    {
        DeltaTrackers[Index].bMetadataDirty = true;
    }
}

void LumenEngine::Engine::FSpatialRegistry::Publish () noexcept
{
    TLockGuard<FMutex> Lock( RegistryMutex );

    /**
     * INFO: We use delta-tracking to only copy changed transforms to the snapshot buffer.
     *       Structural changes (registration/unregistration) still trigger a full copy.
     */
    SnapshotBuffer.WriteBuffer(
        [this] ( FSpatialRegistryData &OutData )
        {
            const USize BufferIdx  = static_cast<USize>( &OutData - SnapshotBuffer.GetBuffers() );
            FDeltaTracker &Tracker = DeltaTrackers[BufferIdx];

            if ( Tracker.bMetadataDirty )
            {
                OutData                = WorkingData;
                Tracker.bMetadataDirty = false;
                Tracker.DirtyIndices.clear();
                Tracker.IsDirty.assign( WorkingData.Transforms.size(), false );
            }
            else
            {
                for ( const USize Index : Tracker.DirtyIndices )
                {
                    OutData.Transforms[Index] = WorkingData.Transforms[Index];
                    Tracker.IsDirty[Index]    = false;
                }
                Tracker.DirtyIndices.clear();
            }
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
