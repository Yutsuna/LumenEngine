/**
 * @file RenderProxyRegistryTests.cpp
 * @brief Functional coverage for canonical proxy snapshot synchronization.
 */

#include <Frame/RenderProxyRegistry.hpp>

#include <Rendering/RenderRecords.hpp>

#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /** @brief Builds a mesh record whose transform sits at InTransformIndex. */
    [[nodiscard]] FRenderMeshRecord
    MakeMesh ( const UInt64 InProxyId, const USize InTransformIndex, const UInt64 InMeshAssetId = 10ULL ) noexcept
    {
        return FRenderMeshRecord{ .ProxyId         = FRenderProxyId( InProxyId ),
                                  .MeshAssetId     = FRenderAssetId( InMeshAssetId ),
                                  .MaterialAssetId = FRenderAssetId( InMeshAssetId + 100ULL ),
                                  .TransformIndex  = InTransformIndex,
                                  .Flags           = ERenderMeshFlags::Visible };
    }

    /** @brief Builds the transform matching MakeMesh at the same index. */
    [[nodiscard]] FRenderTransformRecord MakeTransform ( const UInt64 InProxyId ) noexcept
    {
        return FRenderTransformRecord{ .ProxyId = FRenderProxyId( InProxyId ) };
    }

    /** @brief Views an array of mesh records as an immutable span. */
    template <USize Count>
    [[nodiscard]] TSpan<const FRenderMeshRecord> MeshSpan ( const TArray<FRenderMeshRecord, Count> &InMeshes ) noexcept
    {
        return TSpan<const FRenderMeshRecord>( InMeshes.data(), InMeshes.size() );
    }

    /** @brief Views an array of transform records as an immutable span. */
    template <USize Count>
    [[nodiscard]] TSpan<const FRenderTransformRecord>
    TransformSpan ( const TArray<FRenderTransformRecord, Count> &InTransforms ) noexcept
    {
        return TSpan<const FRenderTransformRecord>( InTransforms.data(), InTransforms.size() );
    }

} // namespace

LUMEN_TEST( FRenderProxyRegistry, EmptySnapshotProducesZeroedStatsAndClearsEntries )
{
    FRenderProxyRegistry Registry( 4ULL );
    FRenderProxyStats Stats{ .Created = 9U, .Updated = 9U, .Removed = 9U, .Rejected = 9U };

    LUMEN_CHECK_HAS_VALUE( Registry.Sync( {}, {}, 1ULL, Stats ) );
    LUMEN_CHECK_EQ( Stats.Created, 0U );
    LUMEN_CHECK_EQ( Stats.Updated, 0U );
    LUMEN_CHECK_EQ( Stats.Removed, 0U );
    LUMEN_CHECK_EQ( Stats.Rejected, 0U );
    LUMEN_CHECK_EQ( Registry.GetSize(), 0ULL );
    LUMEN_CHECK_EQ( Registry.GetCapacity(), 4ULL );
    LUMEN_CHECK_FALSE( Registry.Find( FRenderProxyId( 1ULL ) ).HasValue() );
    LUMEN_CHECK_EQ( Registry.GetEntries().size(), 0ULL );
}

LUMEN_TEST( FRenderProxyRegistry, CanonicalOrderAndSortedDiff )
{
    FRenderProxyRegistry Registry( 8ULL );
    FRenderProxyStats Stats;

    const TArray<FRenderMeshRecord, 3> Meshes{ MakeMesh( 30ULL, 0ULL ), MakeMesh( 10ULL, 1ULL ), MakeMesh( 20ULL, 2ULL ) };
    const TArray<FRenderTransformRecord, 3> Transforms{ MakeTransform( 30ULL ), MakeTransform( 10ULL ), MakeTransform( 20ULL ) };
    LUMEN_CHECK_HAS_VALUE( Registry.Sync( MeshSpan( Meshes ), TransformSpan( Transforms ), 5ULL, Stats ) );
    LUMEN_CHECK_EQ( Stats.Created, 3U );
    LUMEN_CHECK_EQ( Stats.Updated, 0U );
    LUMEN_CHECK_EQ( Stats.Removed, 0U );
    LUMEN_CHECK_EQ( Registry.GetSize(), 3ULL );
    LUMEN_CHECK_EQ( Registry.GetEntries()[0].ProxyId, FRenderProxyId( 10ULL ) );
    LUMEN_CHECK_EQ( Registry.GetEntries()[1].ProxyId, FRenderProxyId( 20ULL ) );
    LUMEN_CHECK_EQ( Registry.GetEntries()[2].ProxyId, FRenderProxyId( 30ULL ) );

    const TArray<FRenderMeshRecord, 2> NextMeshes{ MakeMesh( 40ULL, 0ULL, 77ULL ), MakeMesh( 20ULL, 1ULL, 88ULL ) };
    const TArray<FRenderTransformRecord, 2> NextTransforms{ MakeTransform( 40ULL ), MakeTransform( 20ULL ) };
    LUMEN_CHECK_HAS_VALUE( Registry.Sync( MeshSpan( NextMeshes ), TransformSpan( NextTransforms ), 6ULL, Stats ) );
    LUMEN_CHECK_EQ( Stats.Created, 1U );
    LUMEN_CHECK_EQ( Stats.Updated, 1U );
    LUMEN_CHECK_EQ( Stats.Removed, 2U );
    LUMEN_CHECK_EQ( Registry.GetSize(), 2ULL );
    LUMEN_CHECK_EQ( Registry.GetEntries()[0].ProxyId, FRenderProxyId( 20ULL ) );
    LUMEN_CHECK_EQ( Registry.GetEntries()[1].ProxyId, FRenderProxyId( 40ULL ) );
    LUMEN_CHECK_FALSE( Registry.Find( FRenderProxyId( 30ULL ) ).HasValue() );
    const TOptional<const FRenderProxyEntry *> Updated = Registry.Find( FRenderProxyId( 20ULL ) );
    LUMEN_CHECK_HAS_VALUE( Updated );
    LUMEN_CHECK_EQ( ( *Updated )->MeshAssetId, FRenderAssetId( 88ULL ) );
    LUMEN_CHECK_EQ( ( *Updated )->LastSyncSequence, 6ULL );
    LUMEN_CHECK_EQ( Registry.GetEntries()[1].LastSyncSequence, 6ULL );
}

LUMEN_TEST( FRenderProxyRegistry, RejectedRecordsAreSkippedWithoutAffectingValidOnes )
{
    FRenderProxyRegistry Registry( 4ULL );
    FRenderProxyStats Stats;

    const TArray<FRenderMeshRecord, 6> Meshes{
        MakeMesh( 1ULL, 0ULL ), MakeMesh( 0ULL, 0ULL ), MakeMesh( 2ULL, 0ULL, 0ULL ),
        MakeMesh( 3ULL, 9ULL ), MakeMesh( 4ULL, 1ULL ), MakeMesh( 1ULL, 0ULL, 999ULL ),
    };
    const TArray<FRenderTransformRecord, 2> Transforms{ MakeTransform( 1ULL ), MakeTransform( 1ULL ) };

    LUMEN_CHECK_HAS_VALUE( Registry.Sync( MeshSpan( Meshes ), TransformSpan( Transforms ), 11ULL, Stats ) );
    LUMEN_CHECK_EQ( Stats.Created, 1U );
    LUMEN_CHECK_EQ( Stats.Updated, 0U );
    LUMEN_CHECK_EQ( Stats.Removed, 0U );
    LUMEN_CHECK_EQ( Stats.Rejected, 4U );
    LUMEN_CHECK_EQ( Registry.GetSize(), 1ULL );
    LUMEN_CHECK_EQ( Registry.GetEntries()[0].MeshAssetId, FRenderAssetId( 10ULL ) );
}

LUMEN_TEST( FRenderProxyRegistry, CapacityOverflowLeavesEntriesAndStatsUnchanged )
{
    FRenderProxyRegistry Registry( 2ULL );
    FRenderProxyStats Stats;

    const TArray<FRenderMeshRecord, 2> InitialMeshes{ MakeMesh( 2ULL, 0ULL ), MakeMesh( 1ULL, 1ULL ) };
    const TArray<FRenderTransformRecord, 2> InitialTransforms{ MakeTransform( 2ULL ), MakeTransform( 1ULL ) };
    LUMEN_CHECK_HAS_VALUE( Registry.Sync( MeshSpan( InitialMeshes ), TransformSpan( InitialTransforms ), 7ULL, Stats ) );

    const TArray<FRenderMeshRecord, 3> OverflowMeshes{ MakeMesh( 1ULL, 0ULL ), MakeMesh( 2ULL, 1ULL ), MakeMesh( 3ULL, 2ULL ) };
    const TArray<FRenderTransformRecord, 3> OverflowTransforms{ MakeTransform( 1ULL ), MakeTransform( 2ULL ),
                                                                MakeTransform( 3ULL ) };
    FRenderProxyStats Sentinel{ .Created = 11U, .Updated = 22U, .Removed = 33U, .Rejected = 44U };
    const auto Result = Registry.Sync( MeshSpan( OverflowMeshes ), TransformSpan( OverflowTransforms ), 8ULL, Sentinel );
    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK( Result.Error() == ERenderProxyError::CapacityExceeded );
    LUMEN_CHECK_EQ( Sentinel.Created, 11U );
    LUMEN_CHECK_EQ( Sentinel.Updated, 22U );
    LUMEN_CHECK_EQ( Sentinel.Removed, 33U );
    LUMEN_CHECK_EQ( Sentinel.Rejected, 44U );
    LUMEN_CHECK_EQ( Registry.GetSize(), 2ULL );
    LUMEN_CHECK_EQ( Registry.GetEntries()[0].ProxyId, FRenderProxyId( 1ULL ) );
    LUMEN_CHECK_EQ( Registry.GetEntries()[0].LastSyncSequence, 7ULL );
    LUMEN_CHECK_EQ( Registry.GetEntries()[1].ProxyId, FRenderProxyId( 2ULL ) );

    /** INFO: Duplicate ids count once, so a full registry can still accept their snapshot. */
    const TArray<FRenderMeshRecord, 2> DuplicateMeshes{ MakeMesh( 1ULL, 0ULL ), MakeMesh( 1ULL, 0ULL, 55ULL ) };
    const TArray<FRenderTransformRecord, 1> DuplicateTransforms{ MakeTransform( 1ULL ) };
    LUMEN_CHECK_HAS_VALUE( Registry.Sync( MeshSpan( DuplicateMeshes ), TransformSpan( DuplicateTransforms ), 9ULL, Stats ) );
    LUMEN_CHECK_EQ( Stats.Created, 0U );
    LUMEN_CHECK_EQ( Stats.Updated, 1U );
    LUMEN_CHECK_EQ( Stats.Removed, 1U );
    LUMEN_CHECK_EQ( Registry.GetSize(), 1ULL );
}

LUMEN_TEST( FRenderProxyRegistry, ZeroCapacityRejectsAnyProxy )
{
    FRenderProxyRegistry Registry( 0ULL );
    FRenderProxyStats Stats{ .Created = 1U, .Updated = 2U, .Removed = 3U, .Rejected = 4U };

    LUMEN_CHECK_HAS_VALUE( Registry.Sync( {}, {}, 1ULL, Stats ) );
    LUMEN_CHECK_EQ( Stats.Created, 0U );
    LUMEN_CHECK_EQ( Registry.GetCapacity(), 0ULL );

    const TArray<FRenderMeshRecord, 1> Meshes{ MakeMesh( 1ULL, 0ULL ) };
    const TArray<FRenderTransformRecord, 1> Transforms{ MakeTransform( 1ULL ) };
    FRenderProxyStats Sentinel{ .Created = 1U, .Updated = 2U, .Removed = 3U, .Rejected = 4U };
    const auto Result = Registry.Sync( MeshSpan( Meshes ), TransformSpan( Transforms ), 2ULL, Sentinel );
    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK( Result.Error() == ERenderProxyError::CapacityExceeded );
    LUMEN_CHECK_EQ( Sentinel.Created, 1U );
    LUMEN_CHECK_EQ( Sentinel.Rejected, 4U );
    LUMEN_CHECK_EQ( Registry.GetSize(), 0ULL );
}

LUMEN_TEST( FRenderProxyRegistry, ClearDiscardsAllEntries )
{
    FRenderProxyRegistry Registry( 4ULL );
    FRenderProxyStats Stats;
    const TArray<FRenderMeshRecord, 1> Meshes{ MakeMesh( 7ULL, 0ULL ) };
    const TArray<FRenderTransformRecord, 1> Transforms{ MakeTransform( 7ULL ) };
    LUMEN_CHECK_HAS_VALUE( Registry.Sync( MeshSpan( Meshes ), TransformSpan( Transforms ), 3ULL, Stats ) );
    LUMEN_CHECK_TRUE( Registry.Find( FRenderProxyId( 7ULL ) ).HasValue() );

    Registry.Clear();
    LUMEN_CHECK_EQ( Registry.GetSize(), 0ULL );
    LUMEN_CHECK_FALSE( Registry.Find( FRenderProxyId( 7ULL ) ).HasValue() );
    LUMEN_CHECK_EQ( Registry.GetCapacity(), 4ULL );
}

} // namespace Lumen
