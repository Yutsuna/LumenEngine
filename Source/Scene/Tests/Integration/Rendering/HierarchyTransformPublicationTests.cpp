/**
 * @file HierarchyTransformPublicationTests.cpp
 * @brief World-owned extraction publishes hierarchy-computed world transforms.
 */

#include <World/World.hpp>
#include <World/WorldSpatialRegistry.hpp>
#include <World/WorldTypes.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorTypes.hpp>
#include <Hierarchy/SceneHierarchy.hpp>
#include <Rendering/RenderRecords.hpp>
#include <Rendering/RenderSnapshotView.hpp>

#include <Tests/UnitTest.hpp>

#include <array>
#include <bit>

namespace Lumen
{

namespace
{

    [[nodiscard]] std::array<UInt32, 16ULL> MatrixBits ( const Maths::FMat4x4f &InMatrix ) noexcept
    {
        return std::bit_cast<std::array<UInt32, 16ULL>>( InMatrix );
    }

    [[nodiscard]] FWorldDesc MakePublicationDesc () noexcept
    {
        FWorldDesc Desc;
        Desc.WorkerCount       = 1U;
        Desc.MaxActors         = 16ULL;
        Desc.HierarchyCapacity = 16ULL;
        Desc.WorldId           = 99002ULL;
        Desc.RenderCapacities  = FRenderSnapshotCapacities{
            .TransformCapacity = 16ULL,
            .MeshCapacity      = 16ULL,
            .CameraCapacity    = 0ULL,
            .LightCapacity     = 0ULL,
        };
        return Desc;
    }

    [[nodiscard]] TSharedRef<AActor> SpawnProbe ( FWorld &InWorld )
    {
        class FHierarchyProbeActor final : public AActor
        {
            LUMEN_ACTOR_BODY ( FHierarchyProbeActor );

        public:

            explicit FHierarchyProbeActor ( const ActorID InId ) noexcept : AActor( InId )
            {
            }

            void Receive ( const FMessage &InMessage ) noexcept override
            {
                static_cast<void>( InMessage );
            }
        };

        auto Spawned = InWorld.TrySpawnActor<FHierarchyProbeActor>();
        LUMEN_CHECK_HAS_VALUE( Spawned );
        return *Spawned;
    }

    LUMEN_INTEGRATION_TEST( FHierarchyTransformPublication, SnapshotCarriesHierarchyWorldTransformsCanonically )
    {
        FWorld World( MakePublicationDesc() );

        const TSharedRef<AActor> Parent = SpawnProbe( World );
        const TSharedRef<AActor> Child  = SpawnProbe( World );
        const ActorID ParentId          = Parent->GetId();
        const ActorID ChildId           = Child->GetId();

        LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ParentId ) );
        LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ChildId ) );
        LUMEN_CHECK_HAS_VALUE(
            World.TrySetHierarchyLocalTransform( ParentId, Maths::FMat4x4f::Translate( Maths::FVec3f{ 10.0F, 0.0F, 0.0F } ) ) );
        LUMEN_CHECK_HAS_VALUE(
            World.TrySetHierarchyLocalTransform( ChildId, Maths::FMat4x4f::Translate( Maths::FVec3f{ 1.0F, 0.0F, 0.0F } ) ) );
        LUMEN_CHECK_HAS_VALUE( World.TryHierarchyAttach( ChildId, ParentId ) );
        LUMEN_CHECK_HAS_VALUE( World.GetSpatialRegistry().TryRegisterSpatialEntity( ParentId ) );
        LUMEN_CHECK_HAS_VALUE( World.GetSpatialRegistry().TryRegisterSpatialEntity( ChildId ) );

        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
        const FRenderSnapshotView View = World.GetRenderSnapshot();

        LUMEN_CHECK_EQ( View.Transforms.size(), 2ULL );
        const ActorID FirstId  = static_cast<ActorID>( View.Transforms[0ULL].ProxyId.GetValue() );
        const ActorID SecondId = static_cast<ActorID>( View.Transforms[1ULL].ProxyId.GetValue() );
        LUMEN_CHECK_TRUE( FirstId < SecondId );

        const FSceneHierarchy &Hierarchy = World.GetHierarchy();
        for ( const FRenderTransformRecord &Record : View.Transforms )
        {
            const ActorID Id                          = static_cast<ActorID>( Record.ProxyId.GetValue() );
            const TOptional<Maths::FMat4x4f> Expected = Hierarchy.GetWorldTransform( Id );
            LUMEN_CHECK_TRUE( Expected.HasValue() );
            LUMEN_CHECK_TRUE( MatrixBits( Record.WorldTransform ) ==
                              MatrixBits( Expected.ValueOr( Maths::FMat4x4f::Identity() ) ) );
        }

        LUMEN_CHECK_TRUE( MatrixBits( Hierarchy.GetWorldTransform( ChildId ).ValueOr( Maths::FMat4x4f::Identity() ) ) !=
                          MatrixBits( Hierarchy.GetLocalTransform( ChildId ).ValueOr( Maths::FMat4x4f::Identity() ) ) );
        static_cast<void>( FirstId );
        static_cast<void>( SecondId );
    }

    LUMEN_INTEGRATION_TEST( FHierarchyTransformPublication, RemovalRemainsVisibleAfterSkippedPublications )
    {
        FWorld World( MakePublicationDesc() );

        const TSharedRef<AActor> Parent = SpawnProbe( World );
        const TSharedRef<AActor> Child  = SpawnProbe( World );
        const ActorID ParentId          = Parent->GetId();
        const ActorID ChildId           = Child->GetId();

        LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ParentId ) );
        LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ChildId ) );
        LUMEN_CHECK_HAS_VALUE( World.TryHierarchyAttach( ChildId, ParentId ) );
        LUMEN_CHECK_HAS_VALUE( World.GetSpatialRegistry().TryRegisterSpatialEntity( ParentId ) );
        LUMEN_CHECK_HAS_VALUE( World.GetSpatialRegistry().TryRegisterSpatialEntity( ChildId ) );

        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_HAS_VALUE( World.TryDestroyActor( ChildId ) );
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );

        LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
        const FRenderSnapshotView View = World.GetRenderSnapshot();
        LUMEN_CHECK_EQ( View.Transforms.size(), 1ULL );
        LUMEN_CHECK_EQ( View.Transforms[0ULL].ProxyId.GetValue(), static_cast<UInt64>( ParentId ) );
        LUMEN_CHECK_FALSE( World.GetHierarchy().Contains( ChildId ) );
    }

} // namespace
} // namespace Lumen
