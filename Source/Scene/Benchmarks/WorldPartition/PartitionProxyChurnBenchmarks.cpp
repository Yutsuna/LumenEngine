/**
 * @file PartitionProxyChurnBenchmarks.cpp
 * @brief Renderer-visible proxy churn measured through the immutable snapshot contract only.
 */

#include <Rendering/RenderRecords.hpp>
#include <Rendering/RenderSnapshotStream.hpp>
#include <Rendering/RenderSnapshotTypes.hpp>
#include <Rendering/SceneExtractor.hpp>

#include <Algo/Sort.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

namespace Lumen
{

namespace
{

    [[nodiscard]] FRenderSnapshotCapacities MakeCapacities ( const USize InCount ) noexcept
    {
        return FRenderSnapshotCapacities{
            .TransformCapacity = InCount,
            .MeshCapacity      = InCount,
            .CameraCapacity    = 0ULL,
            .LightCapacity     = 0ULL,
        };
    }

} // namespace

/** @brief Full snapshot publication while proxy identities churn; no actor or RHI type crosses this boundary. */
LUMEN_BENCHMARK_ARGS( FPartitionProxyChurn, Snapshot, 100, 1000, 10000 )
{
    const USize Count     = static_cast<USize>( State.GetArgument() );
    const auto Capacities = MakeCapacities( Count );
    FRenderSnapshotStream Stream( Capacities );
    FSceneExtractor Extractor( Stream );

    TVector<FRenderTransformRecord> Transforms;
    TVector<FRenderMeshRecord> Meshes;
    Transforms.reserve( Count );
    Meshes.reserve( Count );
    for ( USize Index = 0ULL; Index < Count; ++Index )
    {
        Transforms.emplace_back( FRenderTransformRecord{ .ProxyId = FRenderProxyId{ Index + 1ULL } } );
        Meshes.emplace_back( FRenderMeshRecord{
            .ProxyId         = FRenderProxyId{ Index + 1ULL },
            .MeshAssetId     = FRenderAssetId{ Index + 1ULL },
            .MaterialAssetId = FRenderAssetId{ Index + 2ULL },
        } );
    }

    State.SetItemsPerIteration( Count );
    State.SetBytesPerIteration( static_cast<UInt64>( Count ) *
                                ( sizeof( FRenderTransformRecord ) + sizeof( FRenderMeshRecord ) ) );
    UInt64 Sequence = 0ULL;

    for ( auto _ : State )
    {
        const UInt64 Offset = Sequence++ % ( Count + 1ULL );
        for ( USize Index = 0ULL; Index < Count; ++Index )
        {
            const UInt64 Value        = ( ( Index + Offset ) % Count ) + 1ULL;
            Transforms[Index].ProxyId = FRenderProxyId{ Value };
            Meshes[Index].ProxyId     = FRenderProxyId{ Value };
        }
        Algo::Sort( Transforms,
                    [] ( const FRenderTransformRecord &InLeft, const FRenderTransformRecord &InRight ) noexcept -> Bool
        {
            return InLeft.ProxyId.GetValue() < InRight.ProxyId.GetValue();
        } );
        Algo::Sort( Meshes, [] ( const FRenderMeshRecord &InLeft, const FRenderMeshRecord &InRight ) noexcept -> Bool
        {
            return InLeft.ProxyId.GetValue() < InRight.ProxyId.GetValue();
        } );
        for ( USize Index = 0ULL; Index < Count; ++Index )
        {
            Meshes[Index].TransformIndex = Index;
        }

        const FSceneExtractionInput Input{
            .WorldId    = 99120ULL,
            .FrameIndex = Sequence,
            .Transforms = TSpan<const FRenderTransformRecord>{ Transforms.data(), Transforms.size() },
            .Meshes     = TSpan<const FRenderMeshRecord>{ Meshes.data(), Meshes.size() },
        };
        const auto Result = Extractor.Publish( Input );
        Benchmarks::DoNotOptimize( Result );
    }
}

} // namespace Lumen
