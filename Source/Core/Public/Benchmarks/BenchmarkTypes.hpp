/// @file BenchmarkTypes.hpp
/// @brief Defines types and structures used for benchmarking.

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

#include <String/String.hpp>
#include <String/StringView.hpp>

#include <Templates/Vector.hpp>

#include <source_location>

#if defined( __OPTIMIZE__ ) || ( defined( _MSC_VER ) && !defined( _DEBUG ) )
    #define LUMEN_BENCHMARK_OPTIMIZED_BUILD 1
#else
    #define LUMEN_BENCHMARK_OPTIMIZED_BUILD 0
#endif

namespace Lumen
{

namespace Benchmarks
{

    class FBenchmarkState;
    class IBenchmarkReporter;

    /// @brief Signature of a benchmark body.
    using FBenchmarkFunction = void ( * )( FBenchmarkState & );

    ///
    /// @enum EBenchmarkKind
    /// @brief The scale of a benchmark.
    ///
    enum class EBenchmarkKind : UInt8
    {
        /// Fast -> Function, Loop, Container... Iteration is auto-calibrated.
        Micro,
        /// Heavy -> Asset load, level build, frame... One iteration per sample.
        Macro
    };

    ///
    /// @struct FBenchmarkConfig
    /// @brief Measuring strategy of a benchmark.
    ///
    struct FBenchmarkConfig
    {
        /// A sample must run at least this long. 0 disables calibration and uses FixedIterations.
        Float64 MinTimeSeconds{ 0.1 };
        /// Iterations per sample when calibration is disabled.
        UInt64 FixedIterations{ 1 };
        /// Upper bound for the calibrated iteration count.
        UInt64 MaxIterations{ 1'000'000'000 };
        /// Number of timed samples (statistics are computed across them).
        USize Repetitions{ 10 };
        /// Untimed runs executed before sampling (the calibration already warms Micro benchmarks up).
        USize WarmupRuns{ 0 };

        [[nodiscard]] static constexpr FBenchmarkConfig MakeDefaultConfig ( const EBenchmarkKind Kind ) noexcept
        {
            FBenchmarkConfig Config{};
            if ( Kind == EBenchmarkKind::Macro )
            {
                Config.MinTimeSeconds  = 0.0;
                Config.FixedIterations = 1;
                Config.Repetitions     = 5;
                Config.WarmupRuns      = 1;
            }
            return Config;
        }
    };

    ///
    /// @struct FBenchmarkMetadata
    /// @brief Descriptor holding metadata for a benchmark.
    ///
    struct FBenchmarkMetadata
    {
        FStringView SuiteName;
        FStringView BenchmarkName;
        EBenchmarkKind Kind{ EBenchmarkKind::Micro };
        FBenchmarkFunction ExecutionFunction{ nullptr };
        TVector<Int64> Arguments;
        Bool bOptimizedBuild{ true };
        std::source_location DeclaredLocation;
    };

    ///
    /// @struct FBenchmarkStatistics
    /// @brief Distribution of the per-iteration time, across samples, in nanoseconds.
    ///
    struct FBenchmarkStatistics
    {
        Float64 Min{ 0.0 };
        Float64 Max{ 0.0 };
        Float64 Mean{ 0.0 };
        Float64 Median{ 0.0 };
        Float64 StdDev{ 0.0 };
        Float64 CoefficientOfVariation{ 0.0 };
    };

    /// @brief Computes min/max/mean/median/stddev of the given samples.
    [[nodiscard]] CORE_EXPORT FBenchmarkStatistics ComputeStatistics ( TVector<Float64> Samples );

    /// @brief Builds "Suite.Name" or "Suite.Name/Argument".
    [[nodiscard]] CORE_EXPORT FString FormatBenchmarkName ( FStringView SuiteName,
                                                            FStringView BenchmarkName,
                                                            Bool bHasArgument,
                                                            Int64 Argument );

    ///
    /// @struct FBenchmarkResult
    /// @brief Outcome of one benchmark.
    ///
    struct FBenchmarkResult
    {
        FStringView SuiteName;
        FStringView BenchmarkName;
        EBenchmarkKind Kind{ EBenchmarkKind::Micro };
        Bool bHasArgument{ false };
        Int64 Argument{ 0 };

        /// Iterations executed inside each timed sample.
        UInt64 IterationsPerSample{ 0 };
        USize SampleCount{ 0 };
        FBenchmarkStatistics NanosecondsPerIteration{};

        /// Derived from the median; 0 when the benchmark did not declare any.
        Float64 ItemsPerSecond{ 0.0 };
        Float64 BytesPerSecond{ 0.0 };

        Bool bFailed{ false };
        FString FailureMessage;
    };

    ///
    /// @struct FBenchmarkRunInfo
    /// @brief Describes a run before it starts (handed to reporters).
    ///
    struct FBenchmarkRunInfo
    {
        USize BenchmarkCount{ 0 };
        USize LongestNameLength{ 0 };
        Bool bContainsUnoptimizedBenchmarks{ false };
    };

    ///
    /// @struct FBenchmarkRunSummary
    /// @brief Aggregate result of a benchmark execution pass.
    ///
    struct FBenchmarkRunSummary
    {
        USize TotalExecuted{ 0 };
        USize PassedCount{ 0 };
        USize FailedCount{ 0 };
        Float64 DurationSeconds{ 0.0 };

        [[nodiscard]] constexpr Bool IsSuccessful () const noexcept
        {
            return FailedCount == 0;
        }
    };

    ///
    /// @struct FBenchmarkRunOptions
    /// @brief Knobs of a benchmark run.
    ///
    struct FBenchmarkRunOptions
    {
        /// Wildcard (`*`, `?`) or substring filter, matched against "Suite.Name".
        FStringView SuiteFilter;

        Bool bFilterByKind{ false };
        EBenchmarkKind KindFilter{ EBenchmarkKind::Micro };

        /// 0 keeps each benchmark's own repetition count.
        USize RepetitionsOverride{ 0 };
        /// Negative keeps each benchmark's own min time. Handy to speed up CI (e.g. 0.01).
        Float64 MinTimeOverrideSeconds{ -1.0 };

        /// When empty, a FConsoleBenchmarkReporter is used.
        TVector<IBenchmarkReporter *> Reporters;
    };

} // namespace Benchmarks

} // namespace Lumen
