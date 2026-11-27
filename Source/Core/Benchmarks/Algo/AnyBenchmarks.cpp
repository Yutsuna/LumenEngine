/**
 * @file AnyBenchmarks.cpp
 * @brief Timed evidence for the Algo::AnyOf
 */

#include <Algo/Any.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Templates/Vector.hpp>

namespace
{

/** @brief Predicate with no observable side effects, used by both variants. */
[[nodiscard]] Lumen::Bool IsTarget ( const Lumen::Int32 InValue ) noexcept
{
    return InValue == 42;
}

/** @brief Builds a range whose first element matches IsTarget. */
[[nodiscard]] Lumen::TVector<Lumen::Int32> MakeFirstMatchRange ( const Lumen::USize InCount ) noexcept
{
    Lumen::TVector<Lumen::Int32> Values( InCount );

    for ( Lumen::USize ElementIndex = 0U; ElementIndex < InCount; ++ElementIndex )
    {
        Values[ElementIndex] = ( ElementIndex == 0U ) ? 42 : 7;
    }
    return Values;
}

/**
 * Micro
 */

/**
 * AnyOf over a range with no match: full O(n) scalar scan with an
 * equality predicate. Compare with ForEach's auto-vectorised sum.
 */
LUMEN_BENCHMARK_ARGS( AlgoAny, AnyOfNoMatch, 4096, 262144, 4194304 )
{
    const Lumen::USize Length = static_cast<Lumen::USize>( State.GetArgument() );
    const Lumen::TVector<Lumen::Int32> Values( Length, 7 );

    State.SetItemsPerIteration( static_cast<Lumen::UInt64>( Length ) );
    State.SetBytesPerIteration( Length * sizeof( Lumen::Int32 ) );

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Lumen::Algo::AnyOf( Values, IsTarget ) );
    }
}

/**
 * AnyOf with the very first element matching: the loop exits after
 * one predicate call regardless of range size (short-circuit baseline).
 */
LUMEN_BENCHMARK_ARGS( AlgoAny, AnyOfFirstMatch, 4096, 262144, 4194304 )
{
    const Lumen::USize Length                 = static_cast<Lumen::USize>( State.GetArgument() );
    const Lumen::TVector<Lumen::Int32> Values = MakeFirstMatchRange( Length );

    State.SetItemsPerIteration( 1U );

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Lumen::Algo::AnyOf( Values, IsTarget ) );
    }
}

} // namespace
