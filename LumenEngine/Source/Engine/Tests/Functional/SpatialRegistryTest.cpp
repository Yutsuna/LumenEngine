/**
 * @file SpatialRegistryTest.cpp
 * @brief Functional tests for the FSpatialRegistry class and its delta-tracking mechanism.
 */

#include "World/SpatialRegistry.hpp"
#include <gtest/gtest.h>

TEST( EngineWorld, SpatialRegistryLifecycle )
{
    using namespace LumenEngine;
    using namespace LumenEngine::Engine;

    FSpatialRegistry &Registry = FSpatialRegistry::Get();
    const ActorID EntityID     = 42ULL;

    /** Register entity */
    Registry.RegisterSpatialEntity( EntityID );
    Registry.Publish();
    Registry.SwapReadBuffers();

    {
        const FSpatialRegistryData &Snapshot = Registry.GetReadSnapshot();
        const auto It                        = Snapshot.IDToIndex.find( EntityID );

        ASSERT_NE( It, Snapshot.IDToIndex.end() );
        const USize Index = It->second;

        EXPECT_EQ( Snapshot.EntityIDs[Index], EntityID );
        EXPECT_FALSE( Snapshot.Meshes[Index].IsValid() );
        EXPECT_FALSE( Snapshot.Shaders[Index].IsValid() );
    }

    /** Unregister entity */
    Registry.UnregisterSpatialEntity( EntityID );
    Registry.Publish();
    Registry.SwapReadBuffers();

    {
        const FSpatialRegistryData &Snapshot = Registry.GetReadSnapshot();
        EXPECT_EQ( Snapshot.IDToIndex.find( EntityID ), Snapshot.IDToIndex.end() );
    }
}

TEST( EngineWorld, SpatialRegistryDeltaTracking )
{
    using namespace LumenEngine;
    using namespace LumenEngine::Engine;

    FSpatialRegistry &Registry = FSpatialRegistry::Get();
    const ActorID EntityA      = 100ULL;
    const ActorID EntityB      = 200ULL;

    Registry.RegisterSpatialEntity( EntityA );
    Registry.RegisterSpatialEntity( EntityB );

    Maths::FMatrix4x4f TransformA = Maths::FMatrix4x4f::Identity();
    TransformA[0U].X              = 1.5F;

    Registry.UpdateTransform( EntityA, TransformA );

    /**
     * We must publish 3 times to ensure all buffers in the TripleBuffer
     * receive the update and clear their dirty flags.
     */
    for ( USize Index = 0U; Index < 3U; ++Index )
    {
        Registry.Publish();
        Registry.SwapReadBuffers();
        const FSpatialRegistryData &Snapshot = Registry.GetReadSnapshot();

        const auto It = Snapshot.IDToIndex.find( EntityA );
        ASSERT_NE( It, Snapshot.IDToIndex.end() );
        EXPECT_EQ( Snapshot.Transforms[It->second], TransformA );
    }

    /** Update both entities */
    Maths::FMatrix4x4f TransformB = Maths::FMatrix4x4f::Identity();
    TransformB[1U].Y              = 2.5F;
    TransformA[2U].Z              = 3.5F;

    Registry.UpdateTransform( EntityA, TransformA );
    Registry.UpdateTransform( EntityB, TransformB );

    for ( USize Index = 0U; Index < 3U; ++Index )
    {
        Registry.Publish();
        Registry.SwapReadBuffers();
        const FSpatialRegistryData &Snapshot = Registry.GetReadSnapshot();

        const auto ItA = Snapshot.IDToIndex.find( EntityA );
        const auto ItB = Snapshot.IDToIndex.find( EntityB );

        ASSERT_NE( ItA, Snapshot.IDToIndex.end() );
        ASSERT_NE( ItB, Snapshot.IDToIndex.end() );

        EXPECT_EQ( Snapshot.Transforms[ItA->second], TransformA );
        EXPECT_EQ( Snapshot.Transforms[ItB->second], TransformB );
    }

    /** Cleanup */
    Registry.UnregisterSpatialEntity( EntityA );
    Registry.UnregisterSpatialEntity( EntityB );
    Registry.Publish();
}

TEST( EngineWorld, SpatialRegistryRenderData )
{
    using namespace LumenEngine;
    using namespace LumenEngine::Engine;
    using namespace LumenEngine::RHI;

    FSpatialRegistry &Registry = FSpatialRegistry::Get();
    const ActorID EntityID     = 500ULL;
    Registry.RegisterSpatialEntity( EntityID );

    const FMeshHandle Mesh       = FMeshHandle( 10U );
    const FPipelineHandle Shader = FPipelineHandle( 20U );

    Registry.AssignRenderData( EntityID, Mesh, Shader );

    /** Render data assignment triggers bMetadataDirty, so full copy on next Publish. */
    Registry.Publish();
    Registry.SwapReadBuffers();

    {
        const FSpatialRegistryData &Snapshot = Registry.GetReadSnapshot();
        const auto It                        = Snapshot.IDToIndex.find( EntityID );
        ASSERT_NE( It, Snapshot.IDToIndex.end() );

        EXPECT_EQ( Snapshot.Meshes[It->second], Mesh );
        EXPECT_EQ( Snapshot.Shaders[It->second], Shader );
    }

    Registry.UnregisterSpatialEntity( EntityID );
    Registry.Publish();
}
