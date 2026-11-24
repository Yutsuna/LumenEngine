/**
 * @file OptionalBenchmarks.cpp
 * @brief Timed evidence for the TOptional
 */

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Templates/Optional.hpp>

namespace
{

/** @brief Volatile seed: keeps the value opaque to interprocedural constant propagation. */
volatile Lumen::Int32 GOptionalSeed = 42;

/** @brief Boundary crossings per timed iteration: amplifies sub-nanosecond differences. */
constexpr Lumen::Int32 BoundaryCrossings = 64;

/** @brief No-inlinable identity on TOptional: forces the ABI boundary under test. */
LUMEN_NO_INLINE Lumen::TOptional<Lumen::Int32> PassOptional ( const Lumen::TOptional<Lumen::Int32> &InValue ) noexcept
{
    return InValue;
}

/** @brief Same boundary for a trivially copyable scalar (reference measurement). */
LUMEN_NO_INLINE Lumen::Int32 PassRaw ( const Lumen::Int32 InValue ) noexcept
{
    return InValue;
}

} // namespace

/**
 * Micro
 */

/**
 * Micro: F2 — copy-construct TOptional<Int32> across a non-inlined boundary.
 * The comparison includes harness and call costs; it is not a pure ABI ratio.
 */
LUMEN_BENCHMARK( TOptional, ReturnByValue )
{
    const Lumen::Int32 Seed = GOptionalSeed;
    const Lumen::TOptional<Lumen::Int32> Source( Seed );

    State.SetItemsPerIteration( BoundaryCrossings );
    Lumen::Int32 Accumulator = 0;

    for ( auto _ : State )
    {
        for ( Lumen::Int32 CrossingIndex = 0; CrossingIndex < BoundaryCrossings; ++CrossingIndex )
        {
            Accumulator += *PassOptional( Source );
        }
    }

    Lumen::Benchmarks::DoNotOptimize( Accumulator );
}

/**
 * Micro: baseline for ReturnByValue — same boundary, trivial payload.
 */
LUMEN_BENCHMARK( TOptional, RawReturnByValue )
{
    const Lumen::Int32 Seed = GOptionalSeed;

    State.SetItemsPerIteration( BoundaryCrossings );
    Lumen::Int32 Accumulator = 0;

    for ( auto _ : State )
    {
        for ( Lumen::Int32 CrossingIndex = 0; CrossingIndex < BoundaryCrossings; ++CrossingIndex )
        {
            Accumulator += PassRaw( Seed );
        }
    }

    Lumen::Benchmarks::DoNotOptimize( Accumulator );
}
