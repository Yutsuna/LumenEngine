/*
- @file BenchmarkReporter.cpp
- @brief Console, JSON and debug benchmark reporters.
 */

#include <Benchmarks/BenchmarkReporter.hpp>

#include <Core/CoreDefinitions.hpp>

#include <Filesystem/File.hpp>
#include <Filesystem/Path.hpp>
#include <Logging/Logger.hpp>

#include <Meta/Reflection.hpp>
#include <Meta/SerializeToJSON.hpp>

#include <algorithm>
#include <format>
#include <utility>

namespace Lumen
{

LUMEN_LOG_DEFINE_CATEGORY( LogBenchmarks, "Benchmarks" )

namespace Benchmarks
{

    namespace
    {

        [[nodiscard]] FString FormatDuration ( const Float64 Nanoseconds )
        {
            if ( Nanoseconds < 1.0E3 )
            {
                return std::format( "{:.2f} ns", Nanoseconds );
            }
            if ( Nanoseconds < 1.0E6 )
            {
                return std::format( "{:.2f} us", Nanoseconds / 1.0E3 );
            }
            if ( Nanoseconds < 1.0E9 )
            {
                return std::format( "{:.2f} ms", Nanoseconds / 1.0E6 );
            }
            return std::format( "{:.2f} s", Nanoseconds / 1.0E9 );
        }

        [[nodiscard]] FString FormatItemsRate ( const Float64 PerSecond )
        {
            if ( PerSecond >= 1.0E9 )
            {
                return std::format( "{:.2f} G items/s", PerSecond / 1.0E9 );
            }
            if ( PerSecond >= 1.0E6 )
            {
                return std::format( "{:.2f} M items/s", PerSecond / 1.0E6 );
            }
            if ( PerSecond >= 1.0E3 )
            {
                return std::format( "{:.2f} k items/s", PerSecond / 1.0E3 );
            }
            return std::format( "{:.2f} items/s", PerSecond );
        }

        [[nodiscard]] FString FormatBytesRate ( const Float64 PerSecond )
        {
            constexpr Float64 KiB = 1024.0;
            constexpr Float64 MiB = KiB * 1024.0;
            constexpr Float64 GiB = MiB * 1024.0;

            if ( PerSecond >= GiB )
            {
                return std::format( "{:.2f} GiB/s", PerSecond / GiB );
            }
            if ( PerSecond >= MiB )
            {
                return std::format( "{:.2f} MiB/s", PerSecond / MiB );
            }
            if ( PerSecond >= KiB )
            {
                return std::format( "{:.2f} KiB/s", PerSecond / KiB );
            }
            return std::format( "{:.2f} B/s", PerSecond );
        }

        [[nodiscard]] FString FormatThroughput ( const FBenchmarkResult &Result )
        {
            FString Throughput;
            if ( Result.ItemsPerSecond > 0.0 )
            {
                Throughput += FormatItemsRate( Result.ItemsPerSecond );
            }
            if ( Result.BytesPerSecond > 0.0 )
            {
                if ( not Throughput.empty() )
                {
                    Throughput += "  ";
                }
                Throughput += FormatBytesRate( Result.BytesPerSecond );
            }
            if ( Result.NanosecondsPerIteration.CoefficientOfVariation > 10.0 )
            {
                if ( not Throughput.empty() )
                {
                    Throughput += "  ";
                }
                Throughput += "(noisy)";
            }
            return Throughput;
        }

        /**
        - @struct FBenchmarkReport
        - @brief Stable top-level JSON document written by FJsonBenchmarkReporter.
         */
        struct FBenchmarkReport
        {
            Bool bOptimized{ true };
            FBenchmarkRunSummary Summary;
            TVector<FBenchmarkResult> Benchmarks;
        };

    } // namespace

    void FConsoleBenchmarkReporter::OnRunStarted ( const FBenchmarkRunInfo &Info )
    {
        if ( Info.BenchmarkCount == 0 )
        {
            LUMEN_LOG_WARNING( LogBenchmarks, "No benchmark matched the given filter." );
            return;
        }

        if ( Info.bContainsUnoptimizedBenchmarks )
        {
            LUMEN_LOG_WARNING( LogBenchmarks, "Some benchmarks were compiled WITHOUT optimizations: the numbers below are not "
                                              "representative. Reconfigure with --buildtype=release (or debugoptimized)." );
        }

        NameColumnWidth = std::max<USize>( Info.LongestNameLength, 9 );

        FLogger::Flush( std::format( "{:<{}}  {:>12}  {:>12}  {:>7}  {:>22}  {}" LUMEN_NEWLINE, "Benchmark", NameColumnWidth,
                                     "Median/op", "Min/op", "CV", "Iterations x Samples", "Throughput" ) );
        FLogger::Flush( std::format( "{:-<{}}" LUMEN_NEWLINE, "", NameColumnWidth + 80 ) );
    }

    void FConsoleBenchmarkReporter::OnBenchmarkFinished ( const FBenchmarkResult &Result )
    {
        const FString Name = FormatBenchmarkName( Result.SuiteName, Result.BenchmarkName, Result.Argument );

        if ( not Result.Succeeded() )
        {
            const FString Failure = Result.Failure.ValueOr( "unknown failure" );
            FLogger::Flush( std::format( "{:<{}}  FAILED: {}" LUMEN_NEWLINE, Name, NameColumnWidth, Failure ) );
            return;
        }

        const FBenchmarkStatistics &Stats = Result.NanosecondsPerIteration;

        FLogger::Flush( std::format( "{:<{}}  {:>12}  {:>12}  {:>6.1f}%  {:>22}  {}" LUMEN_NEWLINE, Name, NameColumnWidth,
                                     FormatDuration( Stats.Median ), FormatDuration( Stats.Min ), Stats.CoefficientOfVariation,
                                     std::format( "{} x {}", Result.IterationsPerSample, Result.SampleCount ),
                                     FormatThroughput( Result ) ) );
    }

    void FConsoleBenchmarkReporter::OnRunFinished ( const FBenchmarkRunSummary &Summary )
    {
        if ( Summary.TotalExecuted == 0 )
        {
            return;
        }

        if ( Summary.IsSuccessful() )
        {
            LUMEN_LOG_DISPLAY( LogBenchmarks, "{} benchmark(s): {} passed in {:.2f}s", Summary.TotalExecuted, Summary.PassedCount,
                               Summary.DurationSeconds );
        }
        else
        {
            LUMEN_LOG_ERROR( LogBenchmarks, "{} benchmark(s): {} passed, {} failed in {:.2f}s", Summary.TotalExecuted,
                             Summary.PassedCount, Summary.FailedCount, Summary.DurationSeconds );
        }
    }

    FJsonBenchmarkReporter::FJsonBenchmarkReporter ( FString InOutputPath ) : OutputPath{ std::move( InOutputPath ) }
    {
        /** */
    }

    void FJsonBenchmarkReporter::OnRunStarted ( const FBenchmarkRunInfo &Info )
    {
        Results.clear();
        Results.reserve( Info.BenchmarkCount );
        bOptimized = not Info.bContainsUnoptimizedBenchmarks;
    }

    void FJsonBenchmarkReporter::OnBenchmarkFinished ( const FBenchmarkResult &Result )
    {
        Results.push_back( Result );
    }

    void FJsonBenchmarkReporter::OnRunFinished ( const FBenchmarkRunSummary &Summary )
    {
        const FBenchmarkReport Report{
            .bOptimized = bOptimized,
            .Summary    = Summary,
            .Benchmarks = std::move( Results ),
        };

        const FString Json     = Meta::SerializeToJSON( Report );
        const auto WriteResult = Filesystem::FFile::WriteAllText( Filesystem::FPath{ OutputPath }, Json );

        if ( WriteResult )
        {
            LUMEN_LOG_INFO( LogBenchmarks, "Wrote {} benchmark result(s) to '{}'.", Report.Benchmarks.size(), OutputPath );
        }
        else
        {
            LUMEN_LOG_ERROR( LogBenchmarks, "Unable to write '{}': {}.", OutputPath, Meta::EnumName( WriteResult.Error() ) );
        }
    }

    void FDebugBenchmarkReporter::OnRunStarted ( const FBenchmarkRunInfo &Info )
    {
        LUMEN_LOG_VERY_VERBOSE( LogBenchmarks, "{}", Meta::SerializeToJSON( Info, { .bPretty = false } ) );
    }

    void FDebugBenchmarkReporter::OnBenchmarkFinished ( const FBenchmarkResult &Result )
    {
        LUMEN_LOG_VERY_VERBOSE( LogBenchmarks, "{}", Meta::SerializeToJSON( Result, { .bPretty = false } ) );
    }

    void FDebugBenchmarkReporter::OnRunFinished ( const FBenchmarkRunSummary &Summary )
    {
        LUMEN_LOG_VERY_VERBOSE( LogBenchmarks, "{}", Meta::SerializeToJSON( Summary, { .bPretty = false } ) );
    }

} // namespace Benchmarks

} // namespace Lumen
