/**
 * @file WorldRenderExtractionTests.cpp
 * @brief Integration contracts for World-owned immutable render snapshot extraction.
 */

#include <World/World.hpp>
#include <World/WorldRenderContent.hpp>

#include <Actor/Actor.hpp>
#include <Algo/Sort.hpp>
#include <Rendering/RenderSnapshotStream.hpp>
#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

#include <AllocationTracking.hpp>

namespace Lumen
{

namespace
{

    /** @class FExtractionActor
     * @brief Minimal actor used as a committed spatial entity by the integration fixtures.
     */
    class FExtractionActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FExtractionActor );

    public:

        explicit FExtractionActor ( const ActorID InId ) noexcept : AActor( InId )
        {
            /* */
        }

        void Receive ( const FMessage LUMEN_UNUSED &Message ) noexcept override
        {
            /* */
        }
    };

    FWorldDesc MakeDesc ( const UInt64 InWorldId, const USize InCapacity ) noexcept
    {
        FWorldDesc Desc;
        Desc.WorkerCount = 1U;
        /** INFO: Actor capacity exceeds render capacity so a capacity+1 publication can be exercised. */
        Desc.MaxActors        = InCapacity + 4ULL;
        Desc.WorldId          = InWorldId;
        Desc.RenderCapacities = FRenderSnapshotCapacities{
            .TransformCapacity = InCapacity,
            .MeshCapacity      = InCapacity,
            .CameraCapacity    = InCapacity,
            .LightCapacity     = InCapacity,
        };
        return Desc;
    }

    TSharedRef<FExtractionActor> SpawnRegistered ( FWorld &InWorld, const Float32 InOffset )
    {
        TSharedRef<FExtractionActor> Actor = InWorld.SpawnActor<FExtractionActor>();
        LUMEN_CHECK_HAS_VALUE( InWorld.GetSpatialRegistry().TryRegisterSpatialEntity( Actor->GetId() ) );
        InWorld.GetSpatialRegistry().UpdateTransform( Actor->GetId(), Maths::FMat4x4f::Translate( Maths::FVec3f{ InOffset } ) );
        return Actor;
    }

    void CheckPublishedSequence ( FWorld &InWorld, const UInt64 InSequence )
    {
        LUMEN_CHECK_EQ( InWorld.GetRenderSnapshotStream().GetLastPublishedSequence(), InSequence );
        LUMEN_CHECK_EQ( InWorld.GetRenderPublishCount(), InSequence );
    }

    LUMEN_INTEGRATION_TEST( FWorldRenderExtraction, PublishesCanonicalSnapshotAfterTheLastFence )
    {
        FWorld World( MakeDesc( 7001ULL, 3ULL ) );

        /** INFO: Registration order deliberately differs from canonical proxy-ID order. */
        TSharedRef<FExtractionActor> Third  = SpawnRegistered( World, 30.0F );
        TSharedRef<FExtractionActor> First  = SpawnRegistered( World, 10.0F );
        TSharedRef<FExtractionActor> Second = SpawnRegistered( World, 20.0F );

        FRenderMeshRecord Mesh = {
            .ProxyId         = FRenderProxyId{ Second->GetId() },
            .MeshAssetId     = FRenderAssetId{ 900ULL },
            .MaterialAssetId = FRenderAssetId{ 901ULL },
            .TransformIndex  = 0ULL,
            .Flags           = ERenderMeshFlags::Visible | ERenderMeshFlags::CastsShadow,
        };
        World.GetRenderContent().Meshes.emplace_back( Mesh );
        World.GetRenderContent().Cameras.emplace_back(
            FRenderCameraRecord{ .ProxyId = FRenderProxyId{ First->GetId() }, .Flags = ERenderCameraFlags::Active } );
        World.GetRenderContent().Lights.emplace_back(
            FRenderLightRecord{ .ProxyId = FRenderProxyId{ Third->GetId() }, .Intensity = 3.0F, .Position = {} } );

        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_TRUE( World.GetRenderSnapshotStream().HasPendingSnapshot() );

        LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
        const FRenderSnapshotView View = World.GetRenderSnapshot();
        LUMEN_CHECK_EQ( View.Header.WorldId, 7001ULL );
        LUMEN_CHECK_EQ( View.Header.FrameIndex, 1ULL );
        LUMEN_CHECK_EQ( View.Header.SnapshotSequence, 1ULL );
        LUMEN_CHECK_EQ( View.Transforms.size(), 3ULL );
        LUMEN_CHECK_EQ( View.Meshes.size(), 1ULL );
        LUMEN_CHECK_EQ( View.Cameras.size(), 1ULL );
        LUMEN_CHECK_EQ( View.Lights.size(), 1ULL );

        TVector<UInt64> ExpectedIds{ Third->GetId(), First->GetId(), Second->GetId() };
        Algo::Sort( ExpectedIds );
        for ( USize Index = 0ULL; Index < 3ULL; ++Index )
        {
            LUMEN_CHECK_EQ( View.Transforms[Index].ProxyId.GetValue(), ExpectedIds[Index] );
        }
        LUMEN_CHECK_EQ( View.Meshes[0].ProxyId.GetValue(), Second->GetId() );
        USize SecondRank = 0ULL;
        while ( SecondRank < ExpectedIds.size() and ExpectedIds[SecondRank] != Second->GetId() )
        {
            ++SecondRank;
        }
        LUMEN_CHECK_EQ( View.Meshes[0].TransformIndex, SecondRank );
        LUMEN_CHECK_EQ( View.Meshes[0].MeshAssetId.GetValue(), 900ULL );
        LUMEN_CHECK_EQ( View.Cameras[0].ProxyId.GetValue(), First->GetId() );
        LUMEN_CHECK_EQ( View.Lights[0].ProxyId.GetValue(), Third->GetId() );
        CheckPublishedSequence( World, 1ULL );
    }

    LUMEN_INTEGRATION_TEST( FWorldRenderExtraction, DeletionStaysVisibleAfterTwoSkippedPublications )
    {
        FWorld World( MakeDesc( 7002ULL, 2ULL ) );
        TSharedRef<FExtractionActor> Removed = SpawnRegistered( World, 1.0F );
        TSharedRef<FExtractionActor> Kept    = SpawnRegistered( World, 2.0F );

        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_HAS_VALUE( World.TryDestroyActor( Removed->GetId() ) );
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );

        LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
        const FRenderSnapshotView View = World.GetRenderSnapshot();
        LUMEN_CHECK_EQ( View.Header.SnapshotSequence, 3ULL );
        LUMEN_CHECK_EQ( View.Transforms.size(), 1ULL );
        LUMEN_CHECK_EQ( View.Transforms[0].ProxyId.GetValue(), Kept->GetId() );
        LUMEN_CHECK_EQ( World.GetRenderSnapshotStream().GetDroppedSnapshotCount(), 2ULL );
        CheckPublishedSequence( World, 3ULL );
    }

    LUMEN_INTEGRATION_TEST( FWorldRenderExtraction, DestroyWhileAnOldReadSlotIsHeldKeepsThatViewStable )
    {
        FWorld World( MakeDesc( 7003ULL, 3ULL ) );
        TSharedRef<FExtractionActor> Removed = SpawnRegistered( World, 1.0F );
        TSharedRef<FExtractionActor> Kept    = SpawnRegistered( World, 2.0F );

        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
        const FRenderSnapshotView HeldView           = World.GetRenderSnapshot();
        const FRenderTransformRecord *const HeldData = HeldView.Transforms.data();
        LUMEN_CHECK_EQ( HeldView.Transforms.size(), 2ULL );

        LUMEN_CHECK_HAS_VALUE( World.TryDestroyActor( Removed->GetId() ) );
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );

        LUMEN_CHECK_EQ( HeldView.Transforms.size(), 2ULL );
        LUMEN_CHECK_TRUE( HeldView.Transforms.data() == HeldData );
        LUMEN_CHECK_TRUE( HeldView.Transforms[0].ProxyId.IsValid() );

        LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
        const FRenderSnapshotView CurrentView = World.GetRenderSnapshot();
        LUMEN_CHECK_EQ( CurrentView.Transforms.size(), 1ULL );
        LUMEN_CHECK_EQ( CurrentView.Transforms[0].ProxyId.GetValue(), Kept->GetId() );
    }

    LUMEN_INTEGRATION_TEST( FWorldRenderExtraction, TwoWorldsOwnIsolatedStreamsAndSequences )
    {
        FWorld FirstWorld( MakeDesc( 7101ULL, 2ULL ) );
        FWorld SecondWorld( MakeDesc( 7102ULL, 2ULL ) );
        TSharedRef<FExtractionActor> FirstEntity  = SpawnRegistered( FirstWorld, 1.0F );
        TSharedRef<FExtractionActor> SecondEntity = SpawnRegistered( SecondWorld, 2.0F );

        LUMEN_CHECK_HAS_VALUE( FirstWorld.TryTick( 0.016 ) );
        LUMEN_CHECK_HAS_VALUE( SecondWorld.TryTick( 0.016 ) );
        LUMEN_CHECK_TRUE( FirstWorld.SwapRenderSnapshotReadBuffers() );
        LUMEN_CHECK_TRUE( SecondWorld.SwapRenderSnapshotReadBuffers() );

        LUMEN_CHECK_EQ( FirstWorld.GetRenderSnapshot().Header.WorldId, 7101ULL );
        LUMEN_CHECK_EQ( SecondWorld.GetRenderSnapshot().Header.WorldId, 7102ULL );
        LUMEN_CHECK_EQ( FirstWorld.GetRenderSnapshot().Transforms[0].ProxyId.GetValue(), FirstEntity->GetId() );
        LUMEN_CHECK_EQ( SecondWorld.GetRenderSnapshot().Transforms[0].ProxyId.GetValue(), SecondEntity->GetId() );

        LUMEN_CHECK_HAS_VALUE( FirstWorld.TryTick( 0.016 ) );
        LUMEN_CHECK_HAS_VALUE( FirstWorld.TryTick( 0.016 ) );
        LUMEN_CHECK_TRUE( FirstWorld.SwapRenderSnapshotReadBuffers() );
        CheckPublishedSequence( FirstWorld, 3ULL );
        CheckPublishedSequence( SecondWorld, 1ULL );
        LUMEN_CHECK_FALSE( SecondWorld.GetRenderSnapshotStream().HasPendingSnapshot() );
        LUMEN_CHECK_EQ( SecondWorld.GetRenderSnapshotStream().GetDroppedSnapshotCount(), 0ULL );
    }

    LUMEN_INTEGRATION_TEST( FWorldRenderExtraction, CapacityPlusOneRejectsAndKeepsThePreviousSnapshot )
    {
        FWorld World( MakeDesc( 7004ULL, 2ULL ) );
        TSharedRef<FExtractionActor> First  = SpawnRegistered( World, 1.0F );
        TSharedRef<FExtractionActor> Second = SpawnRegistered( World, 2.0F );

        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
        const FRenderSnapshotView Consumed               = World.GetRenderSnapshot();
        const FRenderTransformRecord *const ConsumedData = Consumed.Transforms.data();

        TSharedRef<FExtractionActor> Overflow = SpawnRegistered( World, 3.0F );
        static_cast<void>( Overflow );

        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        const TOptional<FRenderSnapshotError> Error = World.GetLastRenderError();
        LUMEN_CHECK_TRUE( Error.HasValue() );
        LUMEN_CHECK_TRUE( Error->Code == ERenderSnapshotErrorCode::CapacityExceeded );
        LUMEN_CHECK_TRUE( Error->RecordType == ERenderSnapshotRecordType::Transform );
        LUMEN_CHECK_EQ( Error->RequiredCount, 3ULL );
        LUMEN_CHECK_EQ( Error->Capacity, 2ULL );
        LUMEN_CHECK_FALSE( World.GetRenderSnapshotStream().HasPendingSnapshot() );
        LUMEN_CHECK_TRUE( World.GetRenderSnapshot().Transforms.data() == ConsumedData );
        CheckPublishedSequence( World, 1ULL );

        World.GetSpatialRegistry().UnregisterSpatialEntity( Overflow->GetId() );
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_FALSE( World.GetLastRenderError().HasValue() );
        CheckPublishedSequence( World, 2ULL );
        static_cast<void>( First );
        static_cast<void>( Second );
    }

    LUMEN_INTEGRATION_TEST( FWorldRenderExtraction, PublicationAtFixedCapacityPerformsNoAllocation )
    {
        FWorld World( MakeDesc( 7005ULL, 4ULL ) );
        TSharedRef<FExtractionActor> First  = SpawnRegistered( World, 1.0F );
        TSharedRef<FExtractionActor> Second = SpawnRegistered( World, 2.0F );
        TSharedRef<FExtractionActor> Third  = SpawnRegistered( World, 3.0F );
        TSharedRef<FExtractionActor> Fourth = SpawnRegistered( World, 4.0F );

        World.GetRenderContent().Meshes.emplace_back( FRenderMeshRecord{
            .ProxyId = FRenderProxyId{ Fourth->GetId() }, .MeshAssetId = FRenderAssetId{ 42ULL }, .MaterialAssetId = {} } );

        /** INFO: Warm up every scratch array and the triple-buffer slot before measuring. */
        LUMEN_CHECK_HAS_VALUE( World.PublishRenderSnapshot() );

        UInt64 AllocationCount = 0ULL;
        {
            const FAllocationTrackingScope Tracking;
            for ( UInt32 Index = 0U; Index < 1000U; ++Index )
            {
                LUMEN_CHECK_HAS_VALUE( World.PublishRenderSnapshot() );
            }
            AllocationCount = Tracking.GetCount();
        }

        LUMEN_CHECK_EQ( AllocationCount, 0ULL );
        CheckPublishedSequence( World, 1001ULL );
        static_cast<void>( First );
        static_cast<void>( Second );
        static_cast<void>( Third );
    }

    LUMEN_INTEGRATION_TEST( FWorldRenderExtraction, DefaultWorldSkipsExtractionUntilConfigured )
    {
        FWorld World;
        LUMEN_CHECK_FALSE( World.IsRenderExtractionEnabled() );
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_FALSE( World.GetRenderSnapshotStream().HasPendingSnapshot() );
        LUMEN_CHECK_EQ( World.GetRenderSnapshotStream().GetLastPublishedSequence(), 0ULL );
        LUMEN_CHECK_EQ( World.GetRenderPublishCount(), 0ULL );
    }

} // namespace

} // namespace Lumen
