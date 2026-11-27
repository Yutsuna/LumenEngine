/**
 * @file RenderSnapshotTests.cpp
 * @brief Functional tests for canonical extraction and snapshot stream lifetime rules.
 */

#include <Rendering/RenderSnapshotStream.hpp>
#include <Rendering/SceneExtractor.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    [[nodiscard]] FRenderSnapshotCapacities MakeCapacities ( const USize InCount ) noexcept
    {
        return FRenderSnapshotCapacities{
            .TransformCapacity = InCount,
            .MeshCapacity      = InCount,
            .CameraCapacity    = InCount,
            .LightCapacity     = InCount,
        };
    }

    [[nodiscard]] FRenderTransformRecord MakeTransform ( const UInt64 InProxyId, const Float32 InOffset = 0.0F ) noexcept
    {
        return FRenderTransformRecord{
            .ProxyId        = FRenderProxyId{ InProxyId },
            .WorldTransform = Maths::FMat4x4f::Translate( Maths::FVec3f{ InOffset, InOffset, InOffset } ),
            .Bounds         = FRenderBounds{ .Min = Maths::FVec3f{ -1.0F }, .Max = Maths::FVec3f{ 1.0F } },
            .Flags          = ERenderTransformFlags::Visible,
        };
    }

    [[nodiscard]] FRenderMeshRecord MakeMesh ( const UInt64 InProxyId, const USize InTransformIndex ) noexcept
    {
        return FRenderMeshRecord{
            .ProxyId         = FRenderProxyId{ InProxyId },
            .MeshAssetId     = FRenderAssetId{ InProxyId + 1000ULL },
            .MaterialAssetId = FRenderAssetId{ InProxyId + 2000ULL },
            .TransformIndex  = InTransformIndex,
            .Flags           = ERenderMeshFlags::Visible | ERenderMeshFlags::CastsShadow,
        };
    }

    [[nodiscard]] FRenderCameraRecord MakeCamera ( const UInt64 InProxyId, const Bool bActive = false ) noexcept
    {
        return FRenderCameraRecord{
            .ProxyId = FRenderProxyId{ InProxyId },
            .Flags   = bActive ? ERenderCameraFlags::Active : ERenderCameraFlags::None,
        };
    }

    [[nodiscard]] FRenderLightRecord MakeLight ( const UInt64 InProxyId ) noexcept
    {
        return FRenderLightRecord{
            .ProxyId   = FRenderProxyId{ InProxyId },
            .Type      = ERenderLightType::Point,
            .Intensity = static_cast<Float32>( InProxyId ),
        };
    }

    template <typename RecordType>
    [[nodiscard]] TSpan<const RecordType> MakeSpan ( const TVector<RecordType> &InRecords ) noexcept
    {
        return TSpan<const RecordType>{ InRecords.data(), InRecords.size() };
    }

    void CheckCanonicalSnapshot ( const FRenderSnapshotView &InView )
    {
        constexpr UInt64 ExpectedIds[3]      = { 10ULL, 20ULL, 30ULL };
        constexpr Float32 ExpectedOffsets[3] = { 10.0F, 20.0F, 30.0F };

        LUMEN_CHECK_EQ( InView.Transforms.size(), 3ULL );
        LUMEN_CHECK_EQ( InView.Meshes.size(), 3ULL );
        LUMEN_CHECK_EQ( InView.Cameras.size(), 3ULL );
        LUMEN_CHECK_EQ( InView.Lights.size(), 3ULL );

        for ( USize Index = 0ULL; Index < 3ULL; ++Index )
        {
            LUMEN_CHECK_EQ( InView.Transforms[Index].ProxyId.GetValue(), ExpectedIds[Index] );
            LUMEN_CHECK_EQ( InView.Meshes[Index].ProxyId.GetValue(), ExpectedIds[Index] );
            LUMEN_CHECK_EQ( InView.Cameras[Index].ProxyId.GetValue(), ExpectedIds[Index] );
            LUMEN_CHECK_EQ( InView.Lights[Index].ProxyId.GetValue(), ExpectedIds[Index] );
            LUMEN_CHECK_EQ( InView.Meshes[Index].TransformIndex, Index );
            LUMEN_CHECK_EQ( InView.Meshes[Index].MeshAssetId.GetValue(), ExpectedIds[Index] + 1000ULL );
            LUMEN_CHECK_TRUE( InView.Transforms[Index].WorldTransform ==
                              Maths::FMat4x4f::Translate(
                                  Maths::FVec3f{ ExpectedOffsets[Index], ExpectedOffsets[Index], ExpectedOffsets[Index] } ) );
            LUMEN_CHECK_TRUE( InView.Transforms[InView.Meshes[Index].TransformIndex].ProxyId == InView.Meshes[Index].ProxyId );
        }

        USize ActiveCameraCount = 0ULL;
        for ( const FRenderCameraRecord &Camera : InView.Cameras )
        {
            if ( HasAnyFlags( Camera.Flags, ERenderCameraFlags::Active ) )
            {
                if ( ActiveCameraCount == 0ULL )
                {
                    LUMEN_CHECK_EQ( Camera.ProxyId.GetValue(), 10ULL );
                }
                ++ActiveCameraCount;
            }
        }
        LUMEN_CHECK_EQ( ActiveCameraCount, 2ULL );
    }

    LUMEN_TEST( FRenderSnapshotStream, PublishAndSwapExposeOneCompleteSnapshot )
    {
        FRenderSnapshotStream Stream( MakeCapacities( 1ULL ) );
        FSceneExtractor Extractor( Stream );

        const TVector<FRenderTransformRecord> Transforms{ MakeTransform( 42ULL ) };
        const TVector<FRenderMeshRecord> Meshes{ MakeMesh( 42ULL, 0ULL ) };
        const TVector<FRenderCameraRecord> Cameras{ MakeCamera( 42ULL, true ) };
        const TVector<FRenderLightRecord> Lights{ MakeLight( 42ULL ) };
        const FSceneExtractionInput Input{
            .WorldId    = 77ULL,
            .FrameIndex = 9ULL,
            .Transforms = MakeSpan( Transforms ),
            .Meshes     = MakeSpan( Meshes ),
            .Cameras    = MakeSpan( Cameras ),
            .Lights     = MakeSpan( Lights ),
        };

        LUMEN_CHECK_EQ( Stream.GetReadSnapshot().Header.SnapshotSequence, 0ULL );
        auto Result = Extractor.Publish( Input );
        LUMEN_CHECK_HAS_VALUE( Result );
        LUMEN_CHECK_EQ( Result->SnapshotSequence, 1ULL );
        LUMEN_CHECK_EQ( Result->TransformCount, 1ULL );
        LUMEN_CHECK_EQ( Result->MeshCount, 1ULL );
        LUMEN_CHECK_TRUE( Stream.HasPendingSnapshot() );
        LUMEN_CHECK_EQ( Stream.GetReadSnapshot().Header.SnapshotSequence, 0ULL );

        LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );
        const FRenderSnapshotView View = Stream.GetReadSnapshot();
        LUMEN_CHECK_EQ( View.Header.WorldId, 77ULL );
        LUMEN_CHECK_EQ( View.Header.FrameIndex, 9ULL );
        LUMEN_CHECK_EQ( View.Header.SnapshotSequence, 1ULL );
        LUMEN_CHECK_EQ( View.Transforms[0].ProxyId.GetValue(), 42ULL );
        LUMEN_CHECK_EQ( View.Meshes[0].MeshAssetId.GetValue(), 1042ULL );
        LUMEN_CHECK_EQ( View.Cameras[0].ProxyId.GetValue(), 42ULL );
        LUMEN_CHECK_EQ( View.Lights[0].ProxyId.GetValue(), 42ULL );
        LUMEN_CHECK_EQ( Stream.GetLastPublishedSequence(), 1ULL );
        LUMEN_CHECK_EQ( Stream.GetLastConsumedSequence(), 1ULL );
        LUMEN_CHECK_EQ( Stream.GetDroppedSnapshotCount(), 0ULL );

        const FRenderTransformRecord *StableData = View.Transforms.data();
        LUMEN_CHECK_FALSE( Stream.SwapReadBuffers() );
        LUMEN_CHECK_TRUE( Stream.GetReadSnapshot().Transforms.data() == StableData );
    }

    LUMEN_TEST( FRenderSnapshotStream, TwoPublishesAndOneSwapKeepOnlyLatest )
    {
        FRenderSnapshotStream Stream( MakeCapacities( 1ULL ) );
        FSceneExtractor Extractor( Stream );
        TVector<FRenderTransformRecord> Transforms{ MakeTransform( 1ULL ) };
        FSceneExtractionInput Input{
            .WorldId    = 5ULL,
            .FrameIndex = 1ULL,
            .Transforms = MakeSpan( Transforms ),
        };

        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );

        Transforms[0]    = MakeTransform( 2ULL );
        Input.FrameIndex = 2ULL;
        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );
        LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );

        const FRenderSnapshotView View = Stream.GetReadSnapshot();
        LUMEN_CHECK_EQ( View.Header.SnapshotSequence, 2ULL );
        LUMEN_CHECK_EQ( View.Header.FrameIndex, 2ULL );
        LUMEN_CHECK_EQ( View.Transforms.size(), 1ULL );
        LUMEN_CHECK_EQ( View.Transforms[0].ProxyId.GetValue(), 2ULL );
        LUMEN_CHECK_EQ( Stream.GetDroppedSnapshotCount(), 1ULL );
    }

    LUMEN_TEST( FSceneExtractor, CanonicalOrderIsIdenticalAcross32InputPermutations )
    {
        FRenderSnapshotStream Stream( MakeCapacities( 3ULL ) );
        FSceneExtractor Extractor( Stream );

        const FRenderTransformRecord BaseTransforms[3] = {
            MakeTransform( 10ULL, 10.0F ),
            MakeTransform( 20ULL, 20.0F ),
            MakeTransform( 30ULL, 30.0F ),
        };
        const FRenderCameraRecord BaseCameras[3] = {
            MakeCamera( 10ULL, true ),
            MakeCamera( 20ULL ),
            MakeCamera( 30ULL, true ),
        };
        const FRenderLightRecord BaseLights[3] = {
            MakeLight( 10ULL ),
            MakeLight( 20ULL ),
            MakeLight( 30ULL ),
        };
        constexpr UInt64 ProxyIds[3] = { 10ULL, 20ULL, 30ULL };

        for ( USize Permutation = 0ULL; Permutation < 32ULL; ++Permutation )
        {
            FRenderTransformRecord Transforms[3];
            FRenderMeshRecord Meshes[3];
            FRenderCameraRecord Cameras[3];
            FRenderLightRecord Lights[3];

            for ( USize Index = 0ULL; Index < 3ULL; ++Index )
            {
                const USize TransformSource = ( Index + Permutation ) % 3ULL;
                const USize CameraSource    = ( Index + ( Permutation * 2ULL ) ) % 3ULL;
                const USize LightSource     = ( Index + 3ULL - ( Permutation % 3ULL ) ) % 3ULL;
                Transforms[Index]           = BaseTransforms[TransformSource];
                Cameras[Index]              = BaseCameras[CameraSource];
                Lights[Index]               = BaseLights[LightSource];

                const UInt64 MeshProxyId = ProxyIds[( Index + Permutation ) % 3ULL];
                USize TransformIndex     = 0ULL;
                while ( TransformIndex < 3ULL && Transforms[TransformIndex].ProxyId.GetValue() != MeshProxyId )
                {
                    ++TransformIndex;
                }
                Meshes[Index] = MakeMesh( MeshProxyId, TransformIndex );
            }

            const FSceneExtractionInput Input{
                .WorldId    = 99ULL,
                .FrameIndex = Permutation,
                .Transforms = TSpan<const FRenderTransformRecord>{ Transforms, 3ULL },
                .Meshes     = TSpan<const FRenderMeshRecord>{ Meshes, 3ULL },
                .Cameras    = TSpan<const FRenderCameraRecord>{ Cameras, 3ULL },
                .Lights     = TSpan<const FRenderLightRecord>{ Lights, 3ULL },
            };

            auto Result = Extractor.Publish( Input );
            LUMEN_CHECK_HAS_VALUE( Result );
            LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );
            CheckCanonicalSnapshot( Stream.GetReadSnapshot() );
        }

        LUMEN_CHECK_EQ( Stream.GetDroppedSnapshotCount(), 0ULL );
    }

    LUMEN_TEST( FSceneExtractor, CompleteSnapshotsPreserveRemovalAcrossSkippedPublishes )
    {
        FRenderSnapshotStream Stream( MakeCapacities( 3ULL ) );
        FSceneExtractor Extractor( Stream );
        TVector<FRenderTransformRecord> Transforms{ MakeTransform( 1ULL ), MakeTransform( 2ULL ), MakeTransform( 3ULL ) };
        FSceneExtractionInput Input{ .Transforms = MakeSpan( Transforms ) };

        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );
        LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );

        Transforms.erase( Transforms.begin() );
        /** INFO: The extraction input holds non-owning spans; rebuild it after every container mutation. */
        Input.Transforms = MakeSpan( Transforms );
        Input.FrameIndex = 1ULL;
        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );

        Transforms.erase( Transforms.begin() );
        Input.Transforms = MakeSpan( Transforms );
        Input.FrameIndex = 2ULL;
        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );
        LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );

        const FRenderSnapshotView View = Stream.GetReadSnapshot();
        LUMEN_CHECK_EQ( View.Transforms.size(), 1ULL );
        LUMEN_CHECK_EQ( View.Transforms[0].ProxyId.GetValue(), 3ULL );
        LUMEN_CHECK_EQ( View.Header.SnapshotSequence, 3ULL );
        LUMEN_CHECK_EQ( Stream.GetDroppedSnapshotCount(), 1ULL );

        Transforms.clear();
        Input.Transforms = MakeSpan( Transforms );
        Input.FrameIndex = 3ULL;
        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );
        LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );
        LUMEN_CHECK_TRUE( Stream.GetReadSnapshot().Transforms.empty() );
    }

    LUMEN_TEST( FSceneExtractor, CapacityFailureLeavesConsumedAndPendingSnapshotsIntact )
    {
        const FRenderSnapshotCapacities Capacities{ .TransformCapacity = 1ULL };
        FRenderSnapshotStream Stream( Capacities );
        FSceneExtractor Extractor( Stream );
        TVector<FRenderTransformRecord> Transforms{ MakeTransform( 1ULL ) };
        FSceneExtractionInput Input{ .Transforms = MakeSpan( Transforms ) };

        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );
        LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );
        const FRenderTransformRecord *ConsumedData = Stream.GetReadSnapshot().Transforms.data();

        Transforms.emplace_back( MakeTransform( 2ULL ) );
        Input.Transforms      = MakeSpan( Transforms );
        auto ConsumedOverflow = Extractor.Publish( Input );
        LUMEN_CHECK_HAS_ERROR( ConsumedOverflow );
        LUMEN_CHECK_TRUE( ConsumedOverflow.Error().Code == ERenderSnapshotErrorCode::CapacityExceeded );
        LUMEN_CHECK_FALSE( Stream.HasPendingSnapshot() );
        LUMEN_CHECK_EQ( Stream.GetReadSnapshot().Header.SnapshotSequence, 1ULL );
        LUMEN_CHECK_TRUE( Stream.GetReadSnapshot().Transforms.data() == ConsumedData );

        Transforms.pop_back();
        Input.Transforms = MakeSpan( Transforms );
        Transforms[0]    = MakeTransform( 2ULL );
        Input.FrameIndex = 1ULL;
        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );
        LUMEN_CHECK_TRUE( Stream.HasPendingSnapshot() );

        Transforms.emplace_back( MakeTransform( 3ULL ) );
        Input.Transforms     = MakeSpan( Transforms );
        auto PendingOverflow = Extractor.Publish( Input );
        LUMEN_CHECK_HAS_ERROR( PendingOverflow );
        LUMEN_CHECK_EQ( PendingOverflow.Error().RequiredCount, 2ULL );
        LUMEN_CHECK_EQ( PendingOverflow.Error().Capacity, 1ULL );
        LUMEN_CHECK_TRUE( Stream.HasPendingSnapshot() );
        LUMEN_CHECK_EQ( Stream.GetReadSnapshot().Header.SnapshotSequence, 1ULL );

        LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );
        LUMEN_CHECK_EQ( Stream.GetReadSnapshot().Header.SnapshotSequence, 2ULL );
        LUMEN_CHECK_EQ( Stream.GetReadSnapshot().Transforms.size(), 1ULL );
        LUMEN_CHECK_EQ( Stream.GetReadSnapshot().Transforms[0].ProxyId.GetValue(), 2ULL );
        LUMEN_CHECK_EQ( Stream.GetLastPublishedSequence(), 2ULL );
    }

    LUMEN_TEST( FSceneExtractor, EveryRecordFamilyReportsCapacityOverflowAtomically )
    {
        {
            const FRenderSnapshotCapacities Capacities{ .TransformCapacity = 1ULL };
            FRenderSnapshotStream Stream( Capacities );
            FSceneExtractor Extractor( Stream );
            const TVector<FRenderTransformRecord> Transforms{ MakeTransform( 1ULL ) };
            const TVector<FRenderMeshRecord> Meshes{ MakeMesh( 1ULL, 0ULL ) };
            const FSceneExtractionInput Input{ .Transforms = MakeSpan( Transforms ), .Meshes = MakeSpan( Meshes ) };

            auto Result = Extractor.Publish( Input );
            LUMEN_CHECK_HAS_ERROR( Result );
            LUMEN_CHECK_TRUE( Result.Error().RecordType == ERenderSnapshotRecordType::Mesh );
            LUMEN_CHECK_EQ( Result.Error().RequiredCount, 1ULL );
            LUMEN_CHECK_EQ( Result.Error().Capacity, 0ULL );
            LUMEN_CHECK_FALSE( Stream.HasPendingSnapshot() );
            LUMEN_CHECK_EQ( Stream.GetReadSnapshot().Header.SnapshotSequence, 0ULL );
        }

        {
            FRenderSnapshotStream Stream;
            FSceneExtractor Extractor( Stream );
            const TVector<FRenderCameraRecord> Cameras{ MakeCamera( 1ULL ) };
            const FSceneExtractionInput Input{ .Cameras = MakeSpan( Cameras ) };

            auto Result = Extractor.Publish( Input );
            LUMEN_CHECK_HAS_ERROR( Result );
            LUMEN_CHECK_TRUE( Result.Error().RecordType == ERenderSnapshotRecordType::Camera );
            LUMEN_CHECK_FALSE( Stream.HasPendingSnapshot() );
        }

        {
            FRenderSnapshotStream Stream;
            FSceneExtractor Extractor( Stream );
            const TVector<FRenderLightRecord> Lights{ MakeLight( 1ULL ) };
            const FSceneExtractionInput Input{ .Lights = MakeSpan( Lights ) };

            auto Result = Extractor.Publish( Input );
            LUMEN_CHECK_HAS_ERROR( Result );
            LUMEN_CHECK_TRUE( Result.Error().RecordType == ERenderSnapshotRecordType::Light );
            LUMEN_CHECK_FALSE( Stream.HasPendingSnapshot() );
        }
    }

    LUMEN_TEST( FRenderSnapshotStream, HeldViewSurvivesPublishesUntilReaderSwap )
    {
        FRenderSnapshotStream Stream( MakeCapacities( 2ULL ) );
        FSceneExtractor Extractor( Stream );
        TVector<FRenderTransformRecord> Transforms{ MakeTransform( 1ULL ), MakeTransform( 2ULL ) };
        FSceneExtractionInput Input{ .Transforms = MakeSpan( Transforms ) };

        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );
        LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );
        const FRenderSnapshotView HeldView = Stream.GetReadSnapshot();

        Transforms.assign( 1ULL, MakeTransform( 8ULL ) );
        Input.Transforms = MakeSpan( Transforms );
        Input.FrameIndex = 1ULL;
        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );
        Transforms[0]    = MakeTransform( 9ULL );
        Input.FrameIndex = 2ULL;
        LUMEN_CHECK_HAS_VALUE( Extractor.Publish( Input ) );

        LUMEN_CHECK_EQ( HeldView.Header.SnapshotSequence, 1ULL );
        LUMEN_CHECK_EQ( HeldView.Transforms.size(), 2ULL );
        LUMEN_CHECK_EQ( HeldView.Transforms[0].ProxyId.GetValue(), 1ULL );
        LUMEN_CHECK_EQ( HeldView.Transforms[1].ProxyId.GetValue(), 2ULL );
        LUMEN_CHECK_TRUE( Stream.GetReadSnapshot().Transforms.data() == HeldView.Transforms.data() );

        LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );
        const FRenderSnapshotView LatestView = Stream.GetReadSnapshot();
        LUMEN_CHECK_EQ( LatestView.Header.SnapshotSequence, 3ULL );
        LUMEN_CHECK_EQ( LatestView.Transforms.size(), 1ULL );
        LUMEN_CHECK_EQ( LatestView.Transforms[0].ProxyId.GetValue(), 9ULL );
        LUMEN_CHECK_EQ( Stream.GetDroppedSnapshotCount(), 1ULL );
    }

    LUMEN_TEST( FSceneExtractor, InvalidMeshLinksNeverPublishPartialSnapshots )
    {
        FRenderSnapshotStream Stream( MakeCapacities( 1ULL ) );
        FSceneExtractor Extractor( Stream );
        const TVector<FRenderTransformRecord> Transforms{ MakeTransform( 1ULL ) };
        const TVector<FRenderMeshRecord> Meshes{ MakeMesh( 2ULL, 0ULL ) };
        FSceneExtractionInput Input{ .Transforms = MakeSpan( Transforms ), .Meshes = MakeSpan( Meshes ) };

        auto MismatchResult = Extractor.Publish( Input );
        LUMEN_CHECK_HAS_ERROR( MismatchResult );
        LUMEN_CHECK_TRUE( MismatchResult.Error().Code == ERenderSnapshotErrorCode::TransformProxyMismatch );
        LUMEN_CHECK_EQ( MismatchResult.Error().RecordIndex, 0ULL );
        LUMEN_CHECK_FALSE( Stream.HasPendingSnapshot() );

        FRenderMeshRecord InvalidIndexMesh = MakeMesh( 1ULL, 1ULL );
        Input.Meshes                       = TSpan<const FRenderMeshRecord>{ &InvalidIndexMesh, 1ULL };
        auto IndexResult                   = Extractor.Publish( Input );
        LUMEN_CHECK_HAS_ERROR( IndexResult );
        LUMEN_CHECK_TRUE( IndexResult.Error().Code == ERenderSnapshotErrorCode::InvalidTransformIndex );
        LUMEN_CHECK_FALSE( Stream.HasPendingSnapshot() );

        const FRenderMeshRecord ValidMesh = MakeMesh( 1ULL, 0ULL );
        Input.Meshes                      = TSpan<const FRenderMeshRecord>{ &ValidMesh, 1ULL };
        auto ValidResult                  = Extractor.Publish( Input );
        LUMEN_CHECK_HAS_VALUE( ValidResult );
        LUMEN_CHECK_EQ( ValidResult->SnapshotSequence, 1ULL );
    }

} // namespace

} // namespace Lumen
