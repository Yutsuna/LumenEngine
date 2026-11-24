/**
 * @file MonadicBenchmarks.cpp
 * @brief Timed evidence for the FMonadic
 */

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <BenchmarkPayloads.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

#include <utility>

namespace
{

using FPayload          = Lumen::Benchmarks::FHeavyPayload;
using FExpectedPayload  = Lumen::TExpected<FPayload, Lumen::Int32>;
using FCopyOnlyPayload  = Lumen::Benchmarks::FCopyOnlyPayload;
using FCopyOnlyExpected = Lumen::TExpected<FCopyOnlyPayload, Lumen::Int32>;

/** @brief Volatile seed: keeps callback results opaque to constant folding. */
volatile Lumen::Int32 GMonadicSeed = 5;

/** @brief OrElse calls per timed iteration: serialises the copy's atomic increment. */
constexpr Lumen::Int32 OrElseCalls = 64;

/** @brief Produces FPayload by value — the temporary F3 investigates. */
LUMEN_NO_INLINE FPayload MakeHeavyPayload ( const Lumen::Int32 InSeed ) noexcept
{
    return FPayload( InSeed );
}

/** @brief Eagerly computed fallback used by the ValueOr pair.
 *  @details Reads a volatile seed so the compiler cannot hoist the work out of a timed loop.
 */
LUMEN_NO_INLINE Lumen::Int32 MakeFallback ( const Lumen::Int32 InSeed ) noexcept
{
    Lumen::Int32 Result = InSeed;

    for ( Lumen::Int32 Step = 0; Step < 16; ++Step )
    {
        Result = Result * 3 + GMonadicSeed;
    }
    return Result;
}

/** @brief Recovery callback for the copy-vs-move OrElse pair (F10). */
LUMEN_NO_INLINE FCopyOnlyExpected MakeCopyOnlyRecovery ( const Lumen::Int32 &InError ) noexcept
{
    return FCopyOnlyExpected( FCopyOnlyPayload( InError + 1 ) );
}

/** @brief Scalar projection used by the Transform pair for F4. */
[[nodiscard]] Lumen::Int32 ReadPayload ( const FPayload &InPayload ) noexcept
{
    return InPayload.GetValue();
}

} // namespace

/**
 * Micro
 */

/**
 * Micro: F3 baseline — TOptional::Transform uses MonadicEmplace, constructing
 * the callback result directly in the destination storage.
 */
LUMEN_BENCHMARK( FMonadic, TransformOptionalInPlace )
{
    const Lumen::Int32 Seed = GMonadicSeed;
    const Lumen::TOptional<Lumen::Int32> Source( Seed );

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Source.Transform( MakeHeavyPayload ) );
    }
}

/**
 * Micro: F3 — TExpected has no MonadicEmplace hook, so MakeValue builds an
 * Out from the returned temporary, adding one move of the 512-byte payload.
 */
LUMEN_BENCHMARK( FMonadic, TransformExpectedExtraMove )
{
    const Lumen::Int32 Seed = GMonadicSeed;
    const Lumen::TExpected<Lumen::Int32, Lumen::Int32> Source( Seed );

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Source.Transform( MakeHeavyPayload ) );
    }
}

/**
 * Micro: F4 baseline — Transform on an lvalue does not consume the source,
 * so no second move of the container takes place.
 */
LUMEN_BENCHMARK( FMonadic, TransformLValueKeepsSource )
{
    const Lumen::Int32 Seed = GMonadicSeed;

    for ( auto _ : State )
    {
        State.PauseTiming();
        const Lumen::TOptional<FPayload> Source{ FPayload( Seed ) };
        State.ResumeTiming();

        Lumen::Benchmarks::DoNotOptimize( Source.Transform( ReadPayload ) );
    }
}

/**
 * Micro: F4 — Transform on an rvalue move-constructs the `Consumed` guard
 * after building the result, adding one move of the whole source container.
 * The source is rebuilt identically every iteration in both variants.
 */
LUMEN_BENCHMARK( FMonadic, TransformRValueConsumesSource )
{
    const Lumen::Int32 Seed = GMonadicSeed;

    for ( auto _ : State )
    {
        State.PauseTiming();
        Lumen::TOptional<FPayload> Source{ FPayload( Seed ) };
        State.ResumeTiming();

        Lumen::Benchmarks::DoNotOptimize( std::move( Source ).Transform( ReadPayload ) );
    }
}

/**
 * Micro: F9 — ValueOr evaluates its argument before the call, so MakeFallback
 * runs even though Source always holds a value.
 */
LUMEN_BENCHMARK( FMonadic, ValueOrEagerFallback )
{
    const Lumen::Int32 Seed = GMonadicSeed;
    const Lumen::TOptional<Lumen::Int32> Source( Seed );

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Source.ValueOr( MakeFallback( Seed ) ) );
    }
}

/**
 * Micro: F9 baseline — fallback computed once before the timed loop.
 */
LUMEN_BENCHMARK( FMonadic, ValueOrConstantFallback )
{
    const Lumen::Int32 Seed = GMonadicSeed;
    const Lumen::TOptional<Lumen::Int32> Source( Seed );
    const Lumen::Int32 Fallback = MakeFallback( Seed );

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Source.ValueOr( Fallback ) );
    }
}

/**
 * Micro: F10 — OrElse on an lvalue returns TSelfType<Self>, copying the valid
 * container. The payload's copy performs an atomic increment, its move does not.
 */
LUMEN_BENCHMARK( FMonadic, OrElseLValueCopiesContainer )
{
    const Lumen::Int32 Seed = GMonadicSeed;
    const FCopyOnlyExpected Source{ FCopyOnlyPayload( Seed ) };

    State.SetItemsPerIteration( OrElseCalls );

    for ( auto _ : State )
    {
        for ( Lumen::Int32 CallIndex = 0; CallIndex < OrElseCalls; ++CallIndex )
        {
            Lumen::Benchmarks::DoNotOptimize( Source.OrElse( MakeCopyOnlyRecovery ) );
        }
    }
}

/**
 * Micro: F10 baseline — OrElse on an rvalue moves the valid container, so the
 * atomic increment is not paid. Both variants rebuild the source identically.
 */
LUMEN_BENCHMARK( FMonadic, OrElseRValueMovesContainer )
{
    const Lumen::Int32 Seed = GMonadicSeed;

    State.SetItemsPerIteration( OrElseCalls );

    for ( auto _ : State )
    {
        for ( Lumen::Int32 CallIndex = 0; CallIndex < OrElseCalls; ++CallIndex )
        {
            FCopyOnlyExpected Source{ FCopyOnlyPayload( Seed ) };
            Lumen::Benchmarks::DoNotOptimize( std::move( Source ).OrElse( MakeCopyOnlyRecovery ) );
        }
    }
}
