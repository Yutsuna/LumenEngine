/**
 * @file RenderContractTests.cpp
 * @brief Compile-time and baseline contracts for rendering extraction types.
 */

#include <Rendering/RenderIds.hpp>
#include <Rendering/RenderRecords.hpp>
#include <Rendering/RenderSnapshotStorage.hpp>
#include <Rendering/RenderSnapshotStream.hpp>
#include <Rendering/RenderSnapshotView.hpp>
#include <Rendering/SceneExtractor.hpp>

#include <Tests/UnitTest.hpp>

#include <type_traits>
#include <utility>

namespace Lumen
{

namespace
{

    static_assert( Concepts::CTriviallyCopyable<FRenderProxyId> );
    static_assert( Concepts::CTriviallyCopyable<FRenderAssetId> );
    static_assert( Concepts::CTriviallyCopyable<FRenderSnapshotHeader> );
    static_assert( Concepts::CTriviallyCopyable<FRenderTransformRecord> );
    static_assert( Concepts::CTriviallyCopyable<FRenderMeshRecord> );
    static_assert( Concepts::CTriviallyCopyable<FRenderCameraRecord> );
    static_assert( Concepts::CTriviallyCopyable<FRenderLightRecord> );

    static_assert( std::is_standard_layout_v<FRenderProxyId> );
    static_assert( std::is_standard_layout_v<FRenderAssetId> );
    static_assert( std::is_standard_layout_v<FRenderTransformRecord> );
    static_assert( std::is_standard_layout_v<FRenderMeshRecord> );
    static_assert( std::is_standard_layout_v<FRenderCameraRecord> );
    static_assert( std::is_standard_layout_v<FRenderLightRecord> );

    static_assert( not std::is_convertible_v<UInt64, FRenderProxyId> );
    static_assert( not std::is_convertible_v<UInt64, FRenderAssetId> );
    static_assert( not std::is_convertible_v<FRenderProxyId, UInt64> );
    static_assert( not std::is_convertible_v<FRenderAssetId, UInt64> );
    static_assert( not std::is_constructible_v<FRenderProxyId, FRenderAssetId> );
    static_assert( not std::is_constructible_v<FRenderAssetId, FRenderProxyId> );

    static_assert( std::is_same_v<decltype( FRenderSnapshotView::Transforms ), TSpan<const FRenderTransformRecord>> );
    static_assert( std::is_same_v<decltype( FRenderSnapshotView::Meshes ), TSpan<const FRenderMeshRecord>> );
    static_assert( std::is_same_v<decltype( FRenderSnapshotView::Cameras ), TSpan<const FRenderCameraRecord>> );
    static_assert( std::is_same_v<decltype( FRenderSnapshotView::Lights ), TSpan<const FRenderLightRecord>> );

    static_assert( not std::is_copy_constructible_v<FRenderSnapshotStorage> );
    static_assert( not std::is_copy_assignable_v<FRenderSnapshotStorage> );
    static_assert( not std::is_move_constructible_v<FRenderSnapshotStorage> );
    static_assert( not std::is_move_assignable_v<FRenderSnapshotStorage> );
    static_assert( not std::is_copy_constructible_v<FRenderSnapshotStream> );
    static_assert( not std::is_move_constructible_v<FRenderSnapshotStream> );
    static_assert( not std::is_copy_constructible_v<FSceneExtractor> );
    static_assert( not std::is_move_constructible_v<FSceneExtractor> );

    static_assert( noexcept( std::declval<FRenderSnapshotStream &>().SwapReadBuffers() ) );
    static_assert( noexcept( std::declval<const FRenderSnapshotStream &>().GetReadSnapshot() ) );
    static_assert( noexcept( std::declval<FSceneExtractor &>().Publish( std::declval<const FSceneExtractionInput &>() ) ) );

    LUMEN_TEST( FRenderIds, ZeroIsInvalidAndValuesDoNotOwnResources )
    {
        constexpr FRenderProxyId DefaultProxy;
        constexpr FRenderAssetId DefaultAsset;
        constexpr FRenderProxyId Proxy{ 41ULL };
        constexpr FRenderAssetId Asset{ 91ULL };

        static_assert( not DefaultProxy.IsValid() );
        static_assert( not DefaultAsset.IsValid() );
        static_assert( Proxy.IsValid() );
        static_assert( Asset.IsValid() );
        static_assert( Proxy.GetValue() == 41ULL );
        static_assert( Asset.GetValue() == 91ULL );
        static_assert( InvalidRenderProxyId == DefaultProxy );
        static_assert( InvalidRenderAssetId == DefaultAsset );

        LUMEN_CHECK_TRUE( Proxy != FRenderProxyId{ 42ULL } );
        LUMEN_CHECK_TRUE( FRenderProxyId{ 41ULL } < FRenderProxyId{ 42ULL } );
    }

    LUMEN_TEST( FRenderSnapshotStream, ZeroCapacityAcceptsOnlyCompleteEmptySnapshots )
    {
        FRenderSnapshotStream Stream;
        FSceneExtractor Extractor( Stream );

        const FSceneExtractionInput EmptyInput{
            .WorldId    = 7ULL,
            .FrameIndex = 3ULL,
        };

        auto EmptyResult = Extractor.Publish( EmptyInput );
        LUMEN_CHECK_HAS_VALUE( EmptyResult );
        LUMEN_CHECK_EQ( EmptyResult->SnapshotSequence, 1ULL );
        LUMEN_CHECK_TRUE( Stream.SwapReadBuffers() );

        const FRenderSnapshotView EmptyView = Stream.GetReadSnapshot();
        LUMEN_CHECK_EQ( EmptyView.Header.WorldId, 7ULL );
        LUMEN_CHECK_EQ( EmptyView.Header.FrameIndex, 3ULL );
        LUMEN_CHECK_EQ( EmptyView.Transforms.size(), 0ULL );
        LUMEN_CHECK_EQ( EmptyView.Meshes.size(), 0ULL );
        LUMEN_CHECK_EQ( EmptyView.Cameras.size(), 0ULL );
        LUMEN_CHECK_EQ( EmptyView.Lights.size(), 0ULL );

        const FRenderTransformRecord Transform{ .ProxyId = FRenderProxyId{ 1ULL } };
        const FSceneExtractionInput OverflowInput{
            .Transforms = TSpan<const FRenderTransformRecord>{ &Transform, 1ULL },
        };

        auto OverflowResult = Extractor.Publish( OverflowInput );
        LUMEN_CHECK_HAS_ERROR( OverflowResult );
        LUMEN_CHECK_TRUE( OverflowResult.Error().Code == ERenderSnapshotErrorCode::CapacityExceeded );
        LUMEN_CHECK_TRUE( OverflowResult.Error().RecordType == ERenderSnapshotRecordType::Transform );
        LUMEN_CHECK_EQ( OverflowResult.Error().RequiredCount, 1ULL );
        LUMEN_CHECK_EQ( OverflowResult.Error().Capacity, 0ULL );
        LUMEN_CHECK_FALSE( Stream.HasPendingSnapshot() );
        LUMEN_CHECK_EQ( Stream.GetLastPublishedSequence(), 1ULL );

        auto NextEmptyResult = Extractor.Publish( EmptyInput );
        LUMEN_CHECK_HAS_VALUE( NextEmptyResult );
        LUMEN_CHECK_EQ( NextEmptyResult->SnapshotSequence, 2ULL );
    }

} // namespace

} // namespace Lumen
