/*
- @file Benchmark.hpp
- @brief FBenchmarkRegistry definition and registration macros.
 */

#pragma once

#include <CoreExport.hpp>

#include <Benchmarks/BenchmarkState.hpp>
#include <Benchmarks/BenchmarkTypes.hpp>

#include <Templates/Vector.hpp>

#include <algorithm>
#include <source_location>
#include <utility>

namespace Lumen
{

namespace Benchmarks
{

    /**
    - @class FBenchmarkRegistry
    - @brief Registry collecting and executing all statically declared benchmarks.
     */
    class CORE_EXPORT FBenchmarkRegistry final
    {
    public:

        /** @brief Gets the singleton instance of the benchmark registry. */
        [[nodiscard]] static FBenchmarkRegistry &GetInstance () noexcept;

        /** @brief Registers a benchmark with the registry. */
        void Register ( FBenchmarkMetadata Metadata );

        /*
        - @brief Runs the benchmarks with the default console reporter.
        - @param InSuiteFilter Wildcard or substring filter applied to "Suite.Name".
        - @return Benchmark run summary.
         */
        [[nodiscard]] FBenchmarkRunSummary Run ( FStringView InSuiteFilter = {} ) const;

        /** @brief Runs the benchmarks with full control over filtering, overrides and reporters. */
        [[nodiscard]] FBenchmarkRunSummary Run ( const FBenchmarkRunOptions &InOptions ) const;

        /** @return The number of registered benchmarks (an ARGS benchmark counts once). */
        [[nodiscard]] USize GetRegisteredCount () const noexcept;

    private:

        FBenchmarkRegistry () noexcept  = default;
        ~FBenchmarkRegistry () noexcept = default;

        TVector<FBenchmarkMetadata> RegisteredBenchmarks;
    };

    /** @brief Geometric argument list: Min, Min*Multiplier, ... , Max (Max is always included). */
    [[nodiscard]] inline TVector<Int64> MakeRange ( const Int64 Min, const Int64 Max, Int64 Multiplier = 8 )
    {
        TVector<Int64> Values;
        Multiplier = std::max<Lumen::Int64>( Multiplier, 2 );

        Int64 Value = Min > 0 ? Min : 1;
        while ( Value < Max )
        {
            Values.push_back( Value );
            if ( Value > Max / Multiplier )
            {
                break;
            }
            Value *= Multiplier;
        }
        Values.push_back( Max );
        return Values;
    }

    /**
    - @struct FAutoRegisterBenchmark
    - @brief Helper registering a benchmark at dynamic startup.
     */
    struct FAutoRegisterBenchmark
    {
        FAutoRegisterBenchmark ( FStringView InSuite,
                                 FStringView InName,
                                 EBenchmarkKind InKind,
                                 FBenchmarkFunction InFunction,
                                 TVector<Int64> InArguments      = {},
                                 std::source_location InLocation = std::source_location::current() )
        {
            FBenchmarkRegistry::GetInstance().Register( FBenchmarkMetadata{
                .SuiteName         = InSuite,
                .BenchmarkName     = InName,
                .Kind              = InKind,
                .ExecutionFunction = InFunction,
                .Arguments         = std::move( InArguments ),
                .bOptimizedBuild   = LUMEN_BENCHMARK_OPTIMIZED_BUILD != 0,
                .DeclaredLocation  = InLocation,
            } );
        }
    };

} // namespace Benchmarks

} // namespace Lumen

// The body receives a `::Lumen::Benchmarks::FBenchmarkState &State`.

/** Micro benchmark: tiny code, iteration count auto-calibrated. */
#define LUMEN_BENCHMARK( SuiteName, BenchmarkName )                                                                              \
    static void Lumen_Benchmark_##SuiteName##_##BenchmarkName( ::Lumen::Benchmarks::FBenchmarkState &State );                    \
    static inline ::Lumen::Benchmarks::FAutoRegisterBenchmark Lumen_RegisterBenchmark_##SuiteName##_##BenchmarkName(             \
        #SuiteName, #BenchmarkName, ::Lumen::Benchmarks::EBenchmarkKind::Micro,                                                  \
        &Lumen_Benchmark_##SuiteName##_##BenchmarkName );                                                                        \
    static void Lumen_Benchmark_##SuiteName##_##BenchmarkName( ::Lumen::Benchmarks::FBenchmarkState &State )

/** Micro benchmark run once per listed argument: LUMEN_BENCHMARK_ARGS( Suite, Name, 8, 64, 512 ). See State.GetArgument(). */
#define LUMEN_BENCHMARK_ARGS( SuiteName, BenchmarkName, ... )                                                                    \
    static void Lumen_Benchmark_##SuiteName##_##BenchmarkName( ::Lumen::Benchmarks::FBenchmarkState &State );                    \
    static inline ::Lumen::Benchmarks::FAutoRegisterBenchmark Lumen_RegisterBenchmark_##SuiteName##_##BenchmarkName(             \
        #SuiteName, #BenchmarkName, ::Lumen::Benchmarks::EBenchmarkKind::Micro, &Lumen_Benchmark_##SuiteName##_##BenchmarkName,  \
        ::Lumen::TVector<::Lumen::Int64>{ __VA_ARGS__ } );                                                                       \
    static void Lumen_Benchmark_##SuiteName##_##BenchmarkName( ::Lumen::Benchmarks::FBenchmarkState &State )

/** Micro benchmark run for Min, Min*8, Min*64 ... Max. */
#define LUMEN_BENCHMARK_RANGE( SuiteName, BenchmarkName, MinValue, MaxValue )                                                    \
    static void Lumen_Benchmark_##SuiteName##_##BenchmarkName( ::Lumen::Benchmarks::FBenchmarkState &State );                    \
    static inline ::Lumen::Benchmarks::FAutoRegisterBenchmark Lumen_RegisterBenchmark_##SuiteName##_##BenchmarkName(             \
        #SuiteName, #BenchmarkName, ::Lumen::Benchmarks::EBenchmarkKind::Micro, &Lumen_Benchmark_##SuiteName##_##BenchmarkName,  \
        ::Lumen::Benchmarks::MakeRange( MinValue, MaxValue ) );                                                                  \
    static void Lumen_Benchmark_##SuiteName##_##BenchmarkName( ::Lumen::Benchmarks::FBenchmarkState &State )

/** Macro benchmark: heavy code (asset load, level build, a full frame...), one iteration per sample. */
#define LUMEN_MACRO_BENCHMARK( SuiteName, BenchmarkName )                                                                        \
    static void Lumen_MacroBenchmark_##SuiteName##_##BenchmarkName( ::Lumen::Benchmarks::FBenchmarkState &State );               \
    static inline ::Lumen::Benchmarks::FAutoRegisterBenchmark Lumen_RegisterMacroBenchmark_##SuiteName##_##BenchmarkName(        \
        #SuiteName, #BenchmarkName, ::Lumen::Benchmarks::EBenchmarkKind::Macro,                                                  \
        &Lumen_MacroBenchmark_##SuiteName##_##BenchmarkName );                                                                   \
    static void Lumen_MacroBenchmark_##SuiteName##_##BenchmarkName( ::Lumen::Benchmarks::FBenchmarkState &State )
