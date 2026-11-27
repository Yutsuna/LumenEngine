/**
 * @file RenderExtractionBenchmarks.cpp
 * @brief Throughput and copied-byte measurements for World-owned snapshot extraction.
 */

#include <World/World.hpp>
#include <World/WorldRenderContent.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Rendering/RenderRecords.hpp>
#include <Rendering/RenderSnapshotTypes.hpp>
#include <World/WorldSpatialRegistry.hpp>

namespace
{

/** @brief Fixed capacities; zero-allocation publication is asserted by the WorldRenderExtraction integration tests. */
[[nodiscard]] Lumen::FWorldDesc MakeDesc ( const Lumen::USize InCount ) noexcept
{
    Lumen::FWorldDesc Desc;
    Desc.WorkerCount      = 1U;
    Desc.MaxActors        = InCount + 16ULL;
    Desc.WorldId          = 99001ULL;
    Desc.RenderCapacities = Lumen::FRenderSnapshotCapacities{
        .TransformCapacity = InCount,
        .MeshCapacity      = InCount,
        .CameraCapacity    = InCount,
        .LightCapacity     = InCount,
    };
    return Desc;
}

void RegisterRawEntities ( Lumen::FWorld &InWorld, const Lumen::USize InCount ) noexcept
{
    for ( Lumen::USize Index = 0ULL; Index < InCount; ++Index )
    {
        const Lumen::ActorID EntityId = 1ULL + static_cast<Lumen::ActorID>( Index );
        static_cast<void>( InWorld.GetSpatialRegistry().TryRegisterSpatialEntity( EntityId ) );
    }
}

[[nodiscard]] Lumen::UInt64 CopiedBytes ( const Lumen::USize InCount, const Lumen::Bool bWithMeshes ) noexcept
{
    constexpr Lumen::UInt64 HeaderBytes = sizeof( Lumen::FRenderSnapshotHeader );
    const Lumen::UInt64 RecordBytes =
        sizeof( Lumen::FRenderTransformRecord ) + ( bWithMeshes ? sizeof( Lumen::FRenderMeshRecord ) : 0ULL );
    return HeaderBytes + ( static_cast<Lumen::UInt64>( InCount ) * RecordBytes );
}

} // namespace

/** @brief Baseline: publishing an empty complete snapshot at increasing reserved capacities. */
LUMEN_BENCHMARK_ARGS( FWorldRenderExtraction, Idle, 100, 1000, 10000 )
{
    const Lumen::USize Count = static_cast<Lumen::USize>( State.GetArgument() );
    Lumen::FWorld World( MakeDesc( Count ) );

    State.SetItemsPerIteration( 1ULL );
    State.SetBytesPerIteration( CopiedBytes( 0ULL, false ) );

    for ( auto _ : State )
    {
        const auto Result = World.PublishRenderSnapshot();
        Lumen::Benchmarks::DoNotOptimize( Result );
    }
}

/** @brief Every transform is dirty and every proxy carries a mesh; exercises remapping and canonical sorting. */
LUMEN_BENCHMARK_ARGS( FWorldRenderExtraction, AllDirty, 100, 1000, 10000 )
{
    const Lumen::USize Count = static_cast<Lumen::USize>( State.GetArgument() );
    Lumen::FWorld World( MakeDesc( Count ) );
    RegisterRawEntities( World, Count );

    World.GetRenderContent().Meshes.reserve( Count );
    for ( Lumen::USize Index = 0ULL; Index < Count; ++Index )
    {
        const Lumen::FRenderProxyId Proxy{ 1ULL + static_cast<Lumen::UInt64>( Index ) };
        World.GetRenderContent().Meshes.emplace_back( Lumen::FRenderMeshRecord{
            .ProxyId     = Proxy,
            .MeshAssetId = Lumen::FRenderAssetId{ Proxy.GetValue() + 100000ULL },
        } );
    }

    State.SetItemsPerIteration( Count );
    State.SetBytesPerIteration( CopiedBytes( Count, true ) );
    Lumen::UInt64 Sequence = 0ULL;

    for ( auto _ : State )
    {
        ++Sequence;
        const Lumen::Float32 Offset = static_cast<Lumen::Float32>( Sequence % 64U );
        for ( Lumen::USize Index = 0ULL; Index < Count; ++Index )
        {
            World.GetSpatialRegistry().UpdateTransform(
                1ULL + static_cast<Lumen::ActorID>( Index ),
                Lumen::Maths::FMat4x4f::Translate( Lumen::Maths::FVec3f{ Offset, Offset, Offset } ) );
        }

        const auto Result = World.PublishRenderSnapshot();
        Lumen::Benchmarks::DoNotOptimize( Result );
    }
}

/** @brief One retirement and one registration per publish; the complete snapshot must still reflect the change. */
LUMEN_BENCHMARK_ARGS( FWorldRenderExtraction, StructuralChurn, 100, 1000, 10000 )
{
    const Lumen::USize Count = static_cast<Lumen::USize>( State.GetArgument() );
    Lumen::FWorld World( MakeDesc( Count ) );
    RegisterRawEntities( World, Count );

    Lumen::ActorID RotatingId = 1ULL;
    Lumen::UInt64 NextId      = static_cast<Lumen::UInt64>( Count ) + 1ULL;

    State.SetItemsPerIteration( 1ULL );
    State.SetBytesPerIteration( CopiedBytes( Count, false ) );

    for ( auto _ : State )
    {
        World.GetSpatialRegistry().UnregisterSpatialEntity( RotatingId );
        static_cast<void>( World.GetSpatialRegistry().TryRegisterSpatialEntity( NextId ) );
        RotatingId = NextId;
        ++NextId;

        const auto Result = World.PublishRenderSnapshot();
        Lumen::Benchmarks::DoNotOptimize( Result );
    }
}
