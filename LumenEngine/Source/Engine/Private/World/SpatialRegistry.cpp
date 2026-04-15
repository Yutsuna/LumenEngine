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
    const USize LastIndex     = WorkingData.EntityIDs.size() - 1;

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
}

void LumenEngine::Engine::FSpatialRegistry::UpdateTransform ( ActorID InId, const Maths::FMatrix4x4f &InTransform ) noexcept
{
    TLockGuard<FMutex> Lock( RegistryMutex );
    const auto It = WorkingData.IDToIndex.find( InId );

    if ( It == WorkingData.IDToIndex.end() )
    {
        return;
    }
    WorkingData.Transforms[It->second] = InTransform;
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
}

void LumenEngine::Engine::FSpatialRegistry::Publish () noexcept
{
    TLockGuard<FMutex> Lock( RegistryMutex );

    /**
     * INFO: Capacity of the vector and map allocations is inherently reused via the assignment operator
     *       which creates an allocation-free operation on the hot path once size hits the High Water Mark.
     */
    SnapshotBuffer.WriteBuffer( [this] ( FSpatialRegistryData &OutData ) { OutData = WorkingData; } );
}

void LumenEngine::Engine::FSpatialRegistry::SwapReadBuffers () noexcept
{
    SnapshotBuffer.SwapReadBuffers();
}

const LumenEngine::Engine::FSpatialRegistryData &LumenEngine::Engine::FSpatialRegistry::GetReadSnapshot () const noexcept
{
    return SnapshotBuffer.ReadBuffer();
}
