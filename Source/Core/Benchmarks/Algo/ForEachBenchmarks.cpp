/**
 * @file ForEachBenchmarks.cpp
 * @brief Timed evidence for the Algo::ForEach
 */

#include <Algo/ForEach.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Templates/Vector.hpp>

namespace
{

/** @brief Fills a range with Index % 2 so the filter predicate has real work. */
[[nodiscard]] Lumen::TVector<Lumen::Int32> MakeMixedRange ( const Lumen::USize InCount ) noexcept
{
    Lumen::TVector<Lumen::Int32> Values( InCount );

    for ( Lumen::USize ElementIndex = 0U; ElementIndex < InCount; ++ElementIndex )
    {
        Values[ElementIndex] = static_cast<Lumen::Int32>( ElementIndex );
    }
    return Values;
}

} // namespace

/**
 * Micro
 */

/**
 * Micro: F5 reference — ForEach accumulating Int32 with no branch. GCC is
 * expected to auto-vectorise this loop; compare it with Find/AnyOf scans.
 */
LUMEN_BENCHMARK_ARGS( AlgoForEach, SumInt32, 4096, 262144, 4194304 )
{
    const Lumen::USize Length                 = static_cast<Lumen::USize>( State.GetArgument() );
    const Lumen::TVector<Lumen::Int32> Values = MakeMixedRange( Length );

    State.SetItemsPerIteration( static_cast<Lumen::UInt64>( Length ) );
    State.SetBytesPerIteration( static_cast<Lumen::UInt64>( Length * sizeof( Lumen::Int32 ) ) );

    Lumen::Int64 Accumulator = 0;

    for ( auto _ : State )
    {
        Accumulator = 0;

        Lumen::Algo::ForEach( Values, [&Accumulator] ( const Lumen::Int32 InValue ) noexcept
        {
            Accumulator += InValue;
        } );
    }

    Lumen::Benchmarks::DoNotOptimize( Accumulator );
}

/**
 * Micro: F5 — predicate-filtered ForEach: branch per element, no early exit.
 */
LUMEN_BENCHMARK_ARGS( AlgoForEach, FilteredSumInt32, 4096, 262144, 4194304 )
{
    const Lumen::USize Length                 = static_cast<Lumen::USize>( State.GetArgument() );
    const Lumen::TVector<Lumen::Int32> Values = MakeMixedRange( Length );

    State.SetItemsPerIteration( static_cast<Lumen::UInt64>( Length ) );
    State.SetBytesPerIteration( static_cast<Lumen::UInt64>( Length * sizeof( Lumen::Int32 ) ) );

    Lumen::Int64 Accumulator = 0;

    for ( auto _ : State )
    {
        Accumulator = 0;

        Lumen::Algo::ForEach( Values,
                              [] ( const Lumen::Int32 InValue ) noexcept
        {
            return ( InValue & 1 ) == 0;
        }, [&Accumulator] ( const Lumen::Int32 InValue ) noexcept
        {
            Accumulator += InValue;
        } );
    }

    Lumen::Benchmarks::DoNotOptimize( Accumulator );
}

/**
 * Micro: F5 — ForEachUntil over a full miss: same early-exit shape as FindIf,
 * returning nullptr after testing every element.
 */
LUMEN_BENCHMARK_ARGS( AlgoForEach, UntilNoMatch, 4096, 262144, 4194304 )
{
    const Lumen::USize Length                 = static_cast<Lumen::USize>( State.GetArgument() );
    const Lumen::TVector<Lumen::Int32> Values = MakeMixedRange( Length );

    State.SetItemsPerIteration( static_cast<Lumen::UInt64>( Length ) );
    State.SetBytesPerIteration( static_cast<Lumen::UInt64>( Length * sizeof( Lumen::Int32 ) ) );

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Lumen::Algo::ForEachUntil( Values, [] ( const Lumen::Int32 InValue ) noexcept
        {
            return InValue == -1;
        } ) );
    }
}
