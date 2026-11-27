/**
 * @file SceneHierarchyBenchmarks.cpp
 * @brief Bounded hierarchy update, publication and structural churn measurements.
 */

#include <Hierarchy/SceneHierarchy.hpp>

#include <Actor/ActorTypes.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

namespace Lumen
{

namespace
{

    [[nodiscard]] ActorID BenchmarkActor ( const USize InIndex ) noexcept
    {
        return MakeActorId( static_cast<UInt32>( InIndex ) + 1U, 3U );
    }

    [[nodiscard]] TExpected<void, ESceneHierarchyError> RegisterChain ( FSceneHierarchy &InHierarchy,
                                                                        const USize InCount ) noexcept
    {
        for ( USize Index = 0ULL; Index < InCount; ++Index )
        {
            static_cast<void>( InHierarchy.Register( BenchmarkActor( Index ) ) );
            if ( Index > 0ULL )
            {
                static_cast<void>( InHierarchy.Attach( BenchmarkActor( Index ), BenchmarkActor( Index - 1ULL ) ) );
            }
        }
        return {};
    }

    [[nodiscard]] TExpected<void, ESceneHierarchyError> RegisterWideTree ( FSceneHierarchy &InHierarchy,
                                                                           const USize InCount ) noexcept
    {
        constexpr USize Branching = 16ULL;
        static_cast<void>( InHierarchy.Register( BenchmarkActor( 0ULL ) ) );
        for ( USize Index = 1ULL; Index < InCount; ++Index )
        {
            static_cast<void>( InHierarchy.Register( BenchmarkActor( Index ) ) );
            static_cast<void>( InHierarchy.Attach( BenchmarkActor( Index ), BenchmarkActor( ( Index - 1ULL ) / Branching ) ) );
        }
        return {};
    }

    [[nodiscard]] UInt64 PublishedBytes ( const USize InCount ) noexcept
    {
        return static_cast<UInt64>( InCount ) * sizeof( FSceneHierarchyTransformRecord );
    }

} // namespace

/** @brief Chain of 100/1k/10k nodes: root translation dirties the entire subtree. */
LUMEN_BENCHMARK_ARGS( FSceneHierarchy, Chain, 100, 1000, 10000 )
{
    const USize Count = static_cast<USize>( State.GetArgument() );
    FSceneHierarchy Hierarchy;
    static_cast<void>( Hierarchy.Initialize( Count ) );
    static_cast<void>( RegisterChain( Hierarchy, Count ) );
    static_cast<void>( Hierarchy.UpdateWorld() );

    State.SetItemsPerIteration( Count );
    State.SetBytesPerIteration( PublishedBytes( Count ) );
    UInt64 Sequence = 0ULL;

    for ( auto _ : State )
    {
        const Float32 Offset = static_cast<Float32>( ++Sequence % 64U );
        static_cast<void>( Hierarchy.SetLocalTransform( BenchmarkActor( 0ULL ),
                                                        Maths::FMat4x4f::Translate( Maths::FVec3f{ Offset, 0.0F, 0.0F } ) ) );
        const auto Result = Hierarchy.UpdateWorld();
        Benchmarks::DoNotOptimize( Result );
    }
}

/** @brief Wide tree: sibling links and level rebuild dominate. */
LUMEN_BENCHMARK_ARGS( FSceneHierarchy, WideTree, 100, 1000, 10000 )
{
    const USize Count = static_cast<USize>( State.GetArgument() );
    FSceneHierarchy Hierarchy;
    static_cast<void>( Hierarchy.Initialize( Count ) );
    static_cast<void>( RegisterWideTree( Hierarchy, Count ) );
    static_cast<void>( Hierarchy.UpdateWorld() );

    State.SetItemsPerIteration( Count );
    State.SetBytesPerIteration( PublishedBytes( Count ) );
    UInt64 Sequence = 0ULL;

    for ( auto _ : State )
    {
        const Float32 Offset = static_cast<Float32>( ++Sequence % 64U );
        static_cast<void>( Hierarchy.SetLocalTransform( BenchmarkActor( 1ULL ),
                                                        Maths::FMat4x4f::Translate( Maths::FVec3f{ 0.0F, Offset, 0.0F } ) ) );
        const auto Result = Hierarchy.UpdateWorld();
        Benchmarks::DoNotOptimize( Result );
    }
}

/** @brief One leaf mutation per iteration: only that subtree should be recomputed. */
LUMEN_BENCHMARK_ARGS( FSceneHierarchy, LeafUpdates, 100, 1000, 10000 )
{
    const USize Count = static_cast<USize>( State.GetArgument() );
    FSceneHierarchy Hierarchy;
    static_cast<void>( Hierarchy.Initialize( Count ) );
    static_cast<void>( RegisterChain( Hierarchy, Count ) );
    static_cast<void>( Hierarchy.UpdateWorld() );

    State.SetItemsPerIteration( 1ULL );
    State.SetBytesPerIteration( sizeof( FSceneHierarchyTransformRecord ) );
    UInt64 Sequence = 0ULL;

    for ( auto _ : State )
    {
        const Float32 Offset = static_cast<Float32>( ++Sequence % 64U );
        static_cast<void>( Hierarchy.SetLocalTransform( BenchmarkActor( Count - 1ULL ),
                                                        Maths::FMat4x4f::Translate( Maths::FVec3f{ 0.0F, 0.0F, Offset } ) ) );
        const auto Result = Hierarchy.UpdateWorld();
        Benchmarks::DoNotOptimize( Result );
    }
}

/** @brief Full publication of an all-dirty chain. */
LUMEN_BENCHMARK_ARGS( FSceneHierarchy, Publish, 100, 1000, 10000 )
{
    const USize Count = static_cast<USize>( State.GetArgument() );
    FSceneHierarchy Hierarchy;
    static_cast<void>( Hierarchy.Initialize( Count ) );
    static_cast<void>( RegisterChain( Hierarchy, Count ) );

    State.SetItemsPerIteration( Count );
    State.SetBytesPerIteration( PublishedBytes( Count ) );

    for ( auto _ : State )
    {
        const auto Result = Hierarchy.Publish();
        Benchmarks::DoNotOptimize( Result );
    }
}

/** @brief Attach/detach churn at fixed capacity; must never allocate in the timed loop. */
LUMEN_BENCHMARK_ARGS( FSceneHierarchy, StructuralChurn, 100, 1000, 10000 )
{
    const USize Count = static_cast<USize>( State.GetArgument() );
    FSceneHierarchy Hierarchy;
    static_cast<void>( Hierarchy.Initialize( Count ) );
    static_cast<void>( RegisterChain( Hierarchy, Count ) );
    static_cast<void>( Hierarchy.UpdateWorld() );

    State.SetItemsPerIteration( 2ULL );
    State.SetBytesPerIteration( 0ULL );

    for ( auto _ : State )
    {
        static_cast<void>( Hierarchy.Detach( BenchmarkActor( Count - 1ULL ) ) );
        static_cast<void>( Hierarchy.Attach( BenchmarkActor( Count - 1ULL ), BenchmarkActor( Count - 2ULL ) ) );
    }
}

} // namespace Lumen
