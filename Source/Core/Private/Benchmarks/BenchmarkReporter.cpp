/// @file BenchmarkReporter.cpp
/// @brief Console, JSON and debug benchmark reporters.

#include <Benchmarks/BenchmarkReporter.hpp>

#include <Meta/SerializeJSON.hpp>

#include <algorithm>
#include <cstdio>
#include <format>
#include <fstream>
#include <string>
#include <utility>

namespace Lumen
{

namespace Benchmarks
{

    namespace
    {
        void Print ( const std::string &Text )
        {
            std::fwrite( Text.data(), 1, Text.size(), stdout );
            std::fflush( stdout );
        }

        // ---- human-readable layout helpers (NOT serialization) ----

        [[nodiscard]] std::string FormatDuration ( const Float64 Nanoseconds )
        {
            if ( Nanoseconds < 1.0e3 )
            {
                return std::format( "{:.2f} ns", Nanoseconds );
            }
            if ( Nanoseconds < 1.0e6 )
            {
                return std::format( "{:.2f} us", Nanoseconds / 1.0e3 );
            }
            if ( Nanoseconds < 1.0e9 )
            {
                return std::format( "{:.2f} ms", Nanoseconds / 1.0e6 );
            }
            return std::format( "{:.2f} s", Nanoseconds / 1.0e9 );
        }

        [[nodiscard]] std::string FormatItemsRate ( const Float64 PerSecond )
        {
            if ( PerSecond >= 1.0e9 )
            {
                return std::format( "{:.2f} G items/s", PerSecond / 1.0e9 );
            }
            if ( PerSecond >= 1.0e6 )
            {
                return std::format( "{:.2f} M items/s", PerSecond / 1.0e6 );
            }
            if ( PerSecond >= 1.0e3 )
            {
                return std::format( "{:.2f} k items/s", PerSecond / 1.0e3 );
            }
            return std::format( "{:.2f} items/s", PerSecond );
        }

        [[nodiscard]] std::string FormatBytesRate ( const Float64 PerSecond )
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

    } // namespace

    // -------------------------------------------------------------------- console
    // Unchanged: this is layout (columns, units, padding), not serialization.

    void FConsoleBenchmarkReporter::OnRunStarted ( const FBenchmarkRunInfo &Info )
    {
        if ( Info.BenchmarkCount == 0 )
        {
            Print( "No benchmark matched the given filter.\n" );
            return;
        }

        if ( Info.bContainsUnoptimizedBenchmarks )
        {
            Print( "[!] Some benchmarks were compiled WITHOUT optimizations: the numbers below are not representative.\n"
                   "    Reconfigure with --buildtype=release (or debugoptimized).\n\n" );
        }

        NameColumnWidth = std::max<USize>( Info.LongestNameLength, 9 );

        Print( std::format( "{:<{}}  {:>12}  {:>12}  {:>7}  {:>22}  {}\n", "Benchmark", NameColumnWidth, "Median/op", "Min/op",
                            "CV", "Iterations x Samples", "Throughput" ) );
        Print( std::string( NameColumnWidth + 80, '-' ) + "\n" );
    }

    void FConsoleBenchmarkReporter::OnBenchmarkFinished ( const FBenchmarkResult &Result )
    {
        const std::string Name =
            FormatBenchmarkName( Result.SuiteName, Result.BenchmarkName, Result.bHasArgument, Result.Argument );

        if ( Result.bFailed )
        {
            Print( std::format( "{:<{}}  FAILED: {}\n", Name, NameColumnWidth, Result.FailureMessage ) );
            return;
        }

        const FBenchmarkStatistics &Stats = Result.NanosecondsPerIteration;

        std::string Throughput;
        if ( Result.ItemsPerSecond > 0.0 )
        {
            Throughput += FormatItemsRate( Result.ItemsPerSecond );
        }
        if ( Result.BytesPerSecond > 0.0 )
        {
            Throughput += ( Throughput.empty() ? "" : "  " ) + FormatBytesRate( Result.BytesPerSecond );
        }
        if ( Stats.CoefficientOfVariation > 10.0 )
        {
            Throughput += ( Throughput.empty() ? "" : "  " ) + std::string{ "(noisy)" };
        }

        Print( std::format( "{:<{}}  {:>12}  {:>12}  {:>6.1f}%  {:>22}  {}\n", Name, NameColumnWidth,
                            FormatDuration( Stats.Median ), FormatDuration( Stats.Min ), Stats.CoefficientOfVariation,
                            std::format( "{} x {}", Result.IterationsPerSample, Result.SampleCount ), Throughput ) );
    }

    void FConsoleBenchmarkReporter::OnRunFinished ( const FBenchmarkRunSummary &Summary )
    {
        if ( Summary.TotalExecuted == 0 )
        {
            return;
        }

        Print( std::format( "\n{} benchmark(s): {} passed, {} failed in {:.2f}s\n", Summary.TotalExecuted, Summary.PassedCount,
                            Summary.FailedCount, Summary.DurationSeconds ) );
    }

    // -------------------------------------------------------------------- json

    FJsonBenchmarkReporter::FJsonBenchmarkReporter ( std::string InOutputPath ) : OutputPath{ std::move( InOutputPath ) }
    {
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
        // Hand-written schema: stable key order, conditional omission, fixed naming.
        // Meta::SerializeToJSON would tie the schema to FBenchmarkResult's field declaration order.
        std::string Json = "{\n";
        Json += std::format( "  \"optimized\": {},\n", bOptimized ? "true" : "false" );
        Json += std::format( "  \"duration_seconds\": {:.3f},\n", Summary.DurationSeconds );
        Json += "  \"benchmarks\": [\n";

        for ( USize Index = 0; Index < Results.size(); ++Index )
        {
            const FBenchmarkResult &Result = Results[Index];

            Json += "    {\n";
            Json += std::format( "      \"name\": \"{}\",\n",
                                 ToStdView( FormatBenchmarkName( Result.SuiteName, Result.BenchmarkName, Result.bHasArgument,
                                                                 Result.Argument ) ) );
            Json += std::format( "      \"suite\": \"{}\",\n", ToStdView( Result.SuiteName ) );
            Json += std::format( "      \"benchmark\": \"{}\",\n", ToStdView( Result.BenchmarkName ) );
            Json += std::format( "      \"kind\": \"{}\",\n", Meta::EnumName( Result.Kind ) );
            if ( Result.bHasArgument )
            {
                Json += std::format( "      \"argument\": {},\n", Result.Argument );
            }
            Json += std::format( "      \"failed\": {},\n", Result.bFailed ? "true" : "false" );
            if ( Result.bFailed )
            {
                Json += std::format( "      \"error\": \"{}\"\n", ToStdView( Result.FailureMessage ) );
            }
            else
            {
                Json += std::format( "      \"iterations_per_sample\": {},\n", Result.IterationsPerSample );
                Json += std::format( "      \"samples\": {},\n", Result.SampleCount );

                // Sub-object: let Meta own the statistics schema (stable, no conditional omission).
                Json += std::format( "      \"ns_per_iteration\": {},\n",
                                     Meta::SerializeToJSON( Result.NanosecondsPerIteration, { .bPretty = false } ) );

                Json += std::format( "      \"items_per_second\": {:.3f},\n", Result.ItemsPerSecond );
                Json += std::format( "      \"bytes_per_second\": {:.3f}\n", Result.BytesPerSecond );
            }
            Json += ( Index + 1 < Results.size() ) ? "    },\n" : "    }\n";
        }

        Json += "  ]\n}\n";

        std::ofstream File( OutputPath, std::ios::binary | std::ios::trunc );
        if ( not File )
        {
            std::fprintf( stderr, "[Benchmarks] Unable to open '%s' for writing.\n", OutputPath.c_str() );
            return;
        }
        File << Json;
    }

    // -------------------------------------------------------------------- debug

    void FDebugBenchmarkReporter::OnRunStarted ( const FBenchmarkRunInfo &Info )
    {
        Print( std::format( "{}\n", Meta::SerializeToJSON( Info ) ) );
    }

    void FDebugBenchmarkReporter::OnBenchmarkFinished ( const FBenchmarkResult &Result )
    {
        Print( std::format( "{}\n", Meta::SerializeToJSON( Result ) ) );
    }

    void FDebugBenchmarkReporter::OnRunFinished ( const FBenchmarkRunSummary &Summary )
    {
        Print( std::format( "{}\n", Meta::SerializeToJSON( Summary ) ) );
    }

} // namespace Benchmarks

} // namespace Lumen
