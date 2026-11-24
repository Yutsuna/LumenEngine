/**
 * @file FindBenchmarks.cpp
 * @brief Timed evidence for the Algo::Find
 */

#include <Algo/Find.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <BenchmarkPayloads.hpp>

#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace
{

using FPayload = Lumen::Benchmarks::FHeavyPayload;

/** @brief Builds a vector of the requested length with no occurrence of AbsentValue. */
[[nodiscard]] Lumen::TVector<Lumen::Int32> MakeInt32Range ( const Lumen::USize InCount ) noexcept
{
    return Lumen::TVector<Lumen::Int32>( InCount, 7 );
}

/** @brief Builds a vector whose middle element is the only match for the probe. */
[[nodiscard]] Lumen::TVector<FPayload> MakePayloadRange ( const Lumen::USize InCount ) noexcept
{
    Lumen::TVector<FPayload> Values;
    Values.reserve( InCount );

    for ( Lumen::USize ElementIndex = 0U; ElementIndex < InCount; ++ElementIndex )
    {
        Values.emplace_back( static_cast<Lumen::Int32>( ElementIndex ) );
    }
    return Values;
}

/**
 * Micro
 */

/**
 * Iterator-based FindIf over a full miss. The callback is an
 * equality lambda; the loop must test every element before returning InLast.
 */
LUMEN_BENCHMARK_ARGS( AlgoFind, IteratorNoMatch, 4096, 262144, 4194304 )
{
    const Lumen::USize Length                 = static_cast<Lumen::USize>( State.GetArgument() );
    const Lumen::TVector<Lumen::Int32> Values = MakeInt32Range( Length );

    State.SetItemsPerIteration( static_cast<Lumen::UInt64>( Length ) );
    State.SetBytesPerIteration(  Length * sizeof( Lumen::Int32 ) ) ;

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Lumen::Algo::Find( Values.begin(), Values.end(), 42 ) );
    }
}

/**
 * Range-based Find (pointer return) over the same full miss.
 * Same scan, different API; compare with IteratorNoMatch and AnyOfNoMatch.
 */
LUMEN_BENCHMARK_ARGS( AlgoFind, RangeNoMatch, 4096, 262144, 4194304 )
{
    const Lumen::USize Length                 = static_cast<Lumen::USize>( State.GetArgument() );
    const Lumen::TVector<Lumen::Int32> Values = MakeInt32Range( Length );

    State.SetItemsPerIteration( static_cast<Lumen::UInt64>( Length ) );
    State.SetBytesPerIteration(  Length * sizeof( Lumen::Int32 ) );

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Lumen::Algo::Find( Values, 42 ) );
    }
}

/**
 * FindOrNone copies the located element into its TOptional even
 * though only element identity is needed. FPayload makes that copy measurable.
 */
LUMEN_BENCHMARK_ARGS( AlgoFind, OrNoneCopiesElement, 64, 1024, 8192 )
{
    const Lumen::USize Length             = static_cast<Lumen::USize>( State.GetArgument() );
    const Lumen::TVector<FPayload> Values = MakePayloadRange( Length );
    const Lumen::Int32 Target             = static_cast<Lumen::Int32>( Length / 2U );

    State.SetItemsPerIteration( static_cast<Lumen::UInt64>( Length ) );
    State.SetBytesPerIteration( static_cast<Lumen::UInt64>( Length * sizeof( FPayload ) ) );

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Lumen::Algo::FindIfOrNone( Values, [Target] ( const FPayload &InPayload ) noexcept
        {
            return InPayload.Matches( Target );
        } ) );
    }
}

/**
 * Pointer-returning FindIf on the same data and match,
 * with no copy of the located element.
 */
LUMEN_BENCHMARK_ARGS( AlgoFind, PointerNoCopy, 64, 1024, 8192 )
{
    const Lumen::USize Length             = static_cast<Lumen::USize>( State.GetArgument() );
    const Lumen::TVector<FPayload> Values = MakePayloadRange( Length );
    const Lumen::Int32 Target             = static_cast<Lumen::Int32>( Length / 2U );

    State.SetItemsPerIteration( static_cast<Lumen::UInt64>( Length ) );
    State.SetBytesPerIteration( static_cast<Lumen::UInt64>( Length * sizeof( FPayload ) ) );

    for ( auto _ : State )
    {
        Lumen::Benchmarks::DoNotOptimize( Lumen::Algo::FindIf( Values, [Target] ( const FPayload &InPayload ) noexcept
        {
            return InPayload.Matches( Target );
        } ) );
    }
}

}
