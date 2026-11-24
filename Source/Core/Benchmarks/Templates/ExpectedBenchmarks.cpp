/**
 * @file ExpectedBenchmarks.cpp
 * @brief Timed evidence for the TExpected
 */

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Templates/Expected.hpp>

namespace
{

/** @brief Volatile seed: keeps the stored value opaque to the optimiser. */
volatile Lumen::Int32 GExpectedSeed = 7;

/** @brief Volatile validity flag: the compiler cannot prove the expected holds a value. */
volatile Lumen::Int32 GExpectedValidity = 1;

/** @brief Accesses per timed iteration: amplifies sub-nanosecond branch costs. */
constexpr Lumen::Int32 AccessCount = 64;

/**
 * @brief Builds an expected whose state is decided at runtime by InValidity.
 * @details Always succeeds in the benchmark; the opaque branch keeps the assert
 *          in DereferenceExpected from being folded away as redundant.
 */
LUMEN_NO_INLINE Lumen::TExpected<Lumen::Int32, Lumen::Int32> MakeExpected ( const Lumen::Int32 InSeed,
                                                                            const Lumen::Int32 InValidity ) noexcept
{
    if ( InValidity != 0 )
    {
        return Lumen::TExpected<Lumen::Int32, Lumen::Int32>( InSeed );
    }
    return Lumen::TExpected<Lumen::Int32, Lumen::Int32>( Lumen::Unexpect, InSeed );
}

/** @brief No-inlinable identity on TExpected: forces the ABI boundary of F2. */
LUMEN_NO_INLINE Lumen::TExpected<Lumen::Int32, Lumen::Int32>
PassExpected ( const Lumen::TExpected<Lumen::Int32, Lumen::Int32> &InValue ) noexcept
{
    return InValue;
}

/** @brief No-inlinable dereference: LUMEN_ASSERT is checked in Debug and removed under NDEBUG. */
LUMEN_NO_INLINE Lumen::Int32 DereferenceExpected ( const Lumen::TExpected<Lumen::Int32, Lumen::Int32> &InExpected ) noexcept
{
    return *InExpected;
}

/** @brief Baseline dereference of a plain scalar, with no assertion. */
LUMEN_NO_INLINE Lumen::Int32 DereferenceRaw ( const Lumen::Int32 InValue ) noexcept
{
    return InValue;
}

} // namespace

/**
 * Micro
 */

/**
 * Micro: F2 — copy-construct TExpected<Int32, Int32> across a non-inlined boundary.
 */
LUMEN_BENCHMARK( TExpected, ReturnByValue )
{
    const Lumen::Int32 Seed = GExpectedSeed;
    const Lumen::TExpected<Lumen::Int32, Lumen::Int32> Source( Seed );

    State.SetItemsPerIteration( AccessCount );
    Lumen::Int32 Accumulator = 0;

    for ( auto _ : State )
    {
        for ( Lumen::Int32 AccessIndex = 0; AccessIndex < AccessCount; ++AccessIndex )
        {
            Accumulator += *PassExpected( Source );
        }
    }

    Lumen::Benchmarks::DoNotOptimize( Accumulator );
}

/**
 * Micro: F6 — isolated operator* access on a valid expected; the assert and
 * its cold path are measurable because the accessor cannot be inlined.
 */
LUMEN_BENCHMARK( TExpected, DereferenceWithAssert )
{
    const Lumen::Int32 Seed                                  = GExpectedSeed;
    const Lumen::Int32 Validity                              = GExpectedValidity;
    const Lumen::TExpected<Lumen::Int32, Lumen::Int32> Value = MakeExpected( Seed, Validity );

    State.SetItemsPerIteration( AccessCount );
    Lumen::Int32 Accumulator = 0;

    for ( auto _ : State )
    {
        for ( Lumen::Int32 AccessIndex = 0; AccessIndex < AccessCount; ++AccessIndex )
        {
            Accumulator += DereferenceExpected( Value );
        }
    }

    Lumen::Benchmarks::DoNotOptimize( Accumulator );
}

/**
 * Micro: baseline for DereferenceWithAssert — no assert, trivial value.
 */
LUMEN_BENCHMARK( TExpected, DereferenceRaw )
{
    const Lumen::Int32 Seed = GExpectedSeed;

    State.SetItemsPerIteration( AccessCount );
    Lumen::Int32 Accumulator = 0;

    for ( auto _ : State )
    {
        for ( Lumen::Int32 AccessIndex = 0; AccessIndex < AccessCount; ++AccessIndex )
        {
            Accumulator += DereferenceRaw( Seed );
        }
    }

    Lumen::Benchmarks::DoNotOptimize( Accumulator );
}
