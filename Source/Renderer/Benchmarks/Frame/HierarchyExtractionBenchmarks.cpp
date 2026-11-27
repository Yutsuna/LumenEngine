/**
 * @file HierarchyExtractionBenchmarks.cpp
 * @brief Frame extraction stays bounded by published records, not by hierarchy depth.
 */

#include <World/World.hpp>
#include <World/WorldSpatialRegistry.hpp>

#include <Actor/ActorTypes.hpp>
#include <Hierarchy/SceneHierarchy.hpp>
#include <Rendering/RenderRecords.hpp>
#include <Rendering/RenderSnapshotTypes.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

namespace Lumen
{

namespace
{

    [[nodiscard]] FWorldDesc MakeDesc ( const USize InCount ) noexcept
    {
        FWorldDesc Desc;
        Desc.WorkerCount       = 1U;
        Desc.MaxActors         = InCount + 16ULL;
        Desc.HierarchyCapacity = InCount;
        Desc.WorldId           = 99003ULL;
        Desc.RenderCapacities  = FRenderSnapshotCapacities{
             .TransformCapacity = InCount,
             .MeshCapacity      = 0ULL,
             .CameraCapacity    = 0ULL,
             .LightCapacity     = 0ULL,
        };
        return Desc;
    }

    [[nodiscard]] ActorID FrameEntity ( const USize InIndex ) noexcept
    {
        return MakeActorId( static_cast<UInt32>( InIndex ) + 1U, 11U );
    }

    void RegisterHierarchyFrame ( FWorld &InWorld, const USize InCount ) noexcept
    {
        for ( USize Index = 0ULL; Index < InCount; ++Index )
        {
            const ActorID Id = FrameEntity( Index );
            static_cast<void>( InWorld.GetSpatialRegistry().TryRegisterSpatialEntity( Id ) );
            static_cast<void>( InWorld.GetHierarchy().Register( Id ) );
            if ( Index > 0ULL )
            {
                static_cast<void>( InWorld.GetHierarchy().Attach( Id, FrameEntity( Index - 1ULL ) ) );
            }
        }
    }

} // namespace

/** @brief Full tick: hierarchy pass → spatial publication → complete snapshot, at 100/1k/10k nodes. */
LUMEN_BENCHMARK_ARGS( FHierarchyExtraction, BoundedFrame, 100, 1000, 10000 )
{
    const USize Count = static_cast<USize>( State.GetArgument() );
    FWorld World( MakeDesc( Count ) );
    RegisterHierarchyFrame( World, Count );

    State.SetItemsPerIteration( Count );
    State.SetBytesPerIteration( static_cast<UInt64>( Count ) * sizeof( FRenderTransformRecord ) );
    UInt64 Sequence = 0ULL;

    for ( auto _ : State )
    {
        const Float32 Offset = static_cast<Float32>( ++Sequence % 64U );
        static_cast<void>( World.GetHierarchy().SetLocalTransform(
            FrameEntity( 0ULL ), Maths::FMat4x4f::Translate( Maths::FVec3f{ Offset, 0.0F, 0.0F } ) ) );
        const auto Result = World.TryTick( 0.016 );
        Benchmarks::DoNotOptimize( Result );
    }
}

/** @brief Leaf-only updates: extraction cost must not grow with hierarchy depth. */
LUMEN_BENCHMARK_ARGS( FHierarchyExtraction, LeafOnlyFrame, 100, 1000, 10000 )
{
    const USize Count = static_cast<USize>( State.GetArgument() );
    FWorld World( MakeDesc( Count ) );
    RegisterHierarchyFrame( World, Count );

    State.SetItemsPerIteration( 1ULL );
    State.SetBytesPerIteration( static_cast<UInt64>( Count ) * sizeof( FRenderTransformRecord ) );
    UInt64 Sequence = 0ULL;

    for ( auto _ : State )
    {
        const Float32 Offset = static_cast<Float32>( ++Sequence % 64U );
        static_cast<void>( World.GetHierarchy().SetLocalTransform(
            FrameEntity( Count - 1ULL ), Maths::FMat4x4f::Translate( Maths::FVec3f{ 0.0F, 0.0F, Offset } ) ) );
        const auto Result = World.TryTick( 0.016 );
        Benchmarks::DoNotOptimize( Result );
    }
}

} // namespace Lumen
