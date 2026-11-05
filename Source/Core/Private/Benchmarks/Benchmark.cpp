/// @file Benchmark.cpp
/// @brief Benchmark registry, calibration and statistics.

#include <Benchmarks/Benchmark.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <format>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Lumen
{

namespace Benchmarks
{

    FBenchmarkStatistics ComputeStatistics ( std::vector<Float64> Samples )
    {
        FBenchmarkStatistics Stats{};
        if ( Samples.empty() )
        {
            return Stats;
        }

        std::sort( Samples.begin(), Samples.end() );

        const USize Count = Samples.size();
        Stats.Min         = Samples.front();
        Stats.Max         = Samples.back();
        Stats.Median      = ( Count % 2 == 1 ) ? Samples[Count / 2] : ( Samples[Count / 2 - 1] + Samples[Count / 2] ) * 0.5;
        Stats.Mean        = std::accumulate( Samples.begin(), Samples.end(), 0.0 ) / static_cast<Float64>( Count );

        if ( Count > 1 )
        {
            Float64 SquaredDeviations = 0.0;
            for ( const Float64 Sample : Samples )
            {
                SquaredDeviations += ( Sample - Stats.Mean ) * ( Sample - Stats.Mean );
            }
            Stats.StdDev = std::sqrt( SquaredDeviations / static_cast<Float64>( Count - 1 ) );
        }

        Stats.CoefficientOfVariation = Stats.Mean > 0.0 ? Stats.StdDev / Stats.Mean * 100.0 : 0.0;
        return Stats;
    }

    std::string FormatBenchmarkName ( const FStringView SuiteName,
                                      const FStringView BenchmarkName,
                                      const Bool bHasArgument,
                                      const Int64 Argument )
    {
        std::string Name = std::format( "{}.{}", ToStdView( SuiteName ), ToStdView( BenchmarkName ) );
        if ( bHasArgument )
        {
            Name += std::format( "/{}", Argument );
        }
        return Name;
    }

    namespace
    {
        using FClock = std::chrono::steady_clock;

        // ---------------------------------------------------------------- filtering

        [[nodiscard]] Bool WildcardMatch ( const std::string_view Pattern, const std::string_view Text ) noexcept
        {
            constexpr USize NoStar = static_cast<USize>( -1 );

            USize P     = 0;
            USize T     = 0;
            USize StarP = NoStar;
            USize StarT = 0;

            while ( T < Text.size() )
            {
                if ( P < Pattern.size() && ( Pattern[P] == '?' || Pattern[P] == Text[T] ) )
                {
                    ++P;
                    ++T;
                }
                else if ( P < Pattern.size() && Pattern[P] == '*' )
                {
                    StarP = P++;
                    StarT = T;
                }
                else if ( StarP != NoStar )
                {
                    P = StarP + 1;
                    T = ++StarT;
                }
                else
                {
                    return false;
                }
            }

            while ( P < Pattern.size() && Pattern[P] == '*' )
            {
                ++P;
            }
            return P == Pattern.size();
        }

        [[nodiscard]] Bool MatchesFilter ( const FStringView InFilter, const FStringView Suite, const FStringView Name )
        {
            const std::string_view Filter = ToStdView( InFilter );
            if ( Filter.empty() )
            {
                return true;
            }

            const std::string FullName = std::format( "{}.{}", ToStdView( Suite ), ToStdView( Name ) );
            if ( Filter.find_first_of( "*?" ) != std::string_view::npos )
            {
                return WildcardMatch( Filter, ToStdView( Suite ) ) || WildcardMatch( Filter, FullName );
            }
            return FullName.find( Filter ) != std::string::npos;
        }

        // ---------------------------------------------------------------- measuring

        struct FMeasurement
        {
            Float64 ElapsedSeconds{ 0.0 };
            UInt64 Executed{ 0 };
            UInt64 ItemsPerIteration{ 0 };
            UInt64 BytesPerIteration{ 0 };
        };

        [[nodiscard]] FMeasurement
        RunOnce ( const FBenchmarkMetadata &Metadata, const UInt64 Iterations, const Bool bHasArgument, const Int64 Argument )
        {
            FBenchmarkState State{ Iterations, bHasArgument, Argument };
            Metadata.ExecutionFunction( State );
            State.Finish();

            if ( State.GetExecutedIterations() == 0 ) [[unlikely]]
            {
                throw std::runtime_error( "benchmark body never iterated over its state (expected `for ( auto _ : State )`)" );
            }

            return FMeasurement{
                .ElapsedSeconds    = State.GetElapsedSeconds(),
                .Executed          = State.GetExecutedIterations(),
                .ItemsPerIteration = State.GetItemsPerIteration(),
                .BytesPerIteration = State.GetBytesPerIteration(),
            };
        }

        [[nodiscard]] FBenchmarkResult MeasureBenchmark ( const FBenchmarkMetadata &Metadata,
                                                          const Bool bHasArgument,
                                                          const Int64 Argument,
                                                          const FBenchmarkRunOptions &Options )
        {
            FBenchmarkResult Result{};
            Result.SuiteName     = Metadata.SuiteName;
            Result.BenchmarkName = Metadata.BenchmarkName;
            Result.Kind          = Metadata.Kind;
            Result.bHasArgument  = bHasArgument;
            Result.Argument      = Argument;

            FBenchmarkConfig Config = MakeDefaultConfig( Metadata.Kind );
            if ( Options.RepetitionsOverride > 0 )
            {
                Config.Repetitions = Options.RepetitionsOverride;
            }
            if ( Options.MinTimeOverrideSeconds >= 0.0 )
            {
                Config.MinTimeSeconds = Options.MinTimeOverrideSeconds;
            }
            Config.Repetitions = std::max<USize>( Config.Repetitions, 1 );

            try
            {
                // 1. Pick the iteration count (this doubles as the warm-up for calibrated benchmarks).
                UInt64 Iterations = std::max<UInt64>( Config.FixedIterations, 1 );
                if ( Config.MinTimeSeconds > 0.0 )
                {
                    Iterations = 1;
                    for ( ;; )
                    {
                        const FMeasurement Probe = RunOnce( Metadata, Iterations, bHasArgument, Argument );
                        if ( Probe.ElapsedSeconds >= Config.MinTimeSeconds || Iterations >= Config.MaxIterations )
                        {
                            break;
                        }

                        const Float64 PerIteration = Probe.ElapsedSeconds / static_cast<Float64>( Probe.Executed );
                        const Float64 Current      = static_cast<Float64>( Iterations );
                        Float64 Wanted = PerIteration > 0.0 ? Config.MinTimeSeconds * 1.4 / PerIteration : Current * 10.0;
                        Wanted         = std::clamp( Wanted, Current * 2.0, Current * 10.0 );
                        Iterations     = std::min<UInt64>( static_cast<UInt64>( std::ceil( Wanted ) ), Config.MaxIterations );
                    }
                }

                // 2. Optional untimed warm-up runs.
                for ( USize Run = 0; Run < Config.WarmupRuns; ++Run )
                {
                    static_cast<void>( RunOnce( Metadata, Iterations, bHasArgument, Argument ) );
                }

                // 3. Timed samples.
                std::vector<Float64> SamplesNs;
                SamplesNs.reserve( Config.Repetitions );

                UInt64 ItemsPerIteration = 0;
                UInt64 BytesPerIteration = 0;
                for ( USize Run = 0; Run < Config.Repetitions; ++Run )
                {
                    const FMeasurement Sample = RunOnce( Metadata, Iterations, bHasArgument, Argument );
                    SamplesNs.push_back( Sample.ElapsedSeconds * 1.0e9 / static_cast<Float64>( Sample.Executed ) );
                    ItemsPerIteration = Sample.ItemsPerIteration;
                    BytesPerIteration = Sample.BytesPerIteration;
                }

                Result.IterationsPerSample     = Iterations;
                Result.SampleCount             = SamplesNs.size();
                Result.NanosecondsPerIteration = ComputeStatistics( std::move( SamplesNs ) );

                const Float64 MedianNs = Result.NanosecondsPerIteration.Median;
                if ( MedianNs > 0.0 )
                {
                    Result.ItemsPerSecond = static_cast<Float64>( ItemsPerIteration ) * 1.0e9 / MedianNs;
                    Result.BytesPerSecond = static_cast<Float64>( BytesPerIteration ) * 1.0e9 / MedianNs;
                }
            }
            catch ( const std::exception &Exception )
            {
                Result.bFailed        = true;
                Result.FailureMessage = std::format( "{}", Exception.what() );
            }
            catch ( ... )
            {
                Result.bFailed        = true;
                Result.FailureMessage = "unknown exception";
            }

            return Result;
        }

        struct FPlannedRun
        {
            const FBenchmarkMetadata *Metadata{ nullptr };
            Bool bHasArgument{ false };
            Int64 Argument{ 0 };
        };
    } // namespace

    // -------------------------------------------------------------------- registry

    FBenchmarkRegistry &FBenchmarkRegistry::GetInstance () noexcept
    {
        static FBenchmarkRegistry Instance;
        return Instance;
    }

    void FBenchmarkRegistry::Register ( FBenchmarkMetadata Metadata )
    {
        RegisteredBenchmarks.push_back( std::move( Metadata ) );
    }

    USize FBenchmarkRegistry::GetRegisteredCount () const noexcept
    {
        return RegisteredBenchmarks.size();
    }

    FBenchmarkRunSummary FBenchmarkRegistry::Run ( const FStringView InSuiteFilter ) const
    {
        FBenchmarkRunOptions Options{};
        Options.SuiteFilter = InSuiteFilter;
        return Run( Options );
    }

    FBenchmarkRunSummary FBenchmarkRegistry::Run ( const FBenchmarkRunOptions &InOptions ) const
    {
        FConsoleBenchmarkReporter DefaultReporter;
        std::vector<IBenchmarkReporter *> Reporters = InOptions.Reporters;
        if ( Reporters.empty() )
        {
            Reporters.push_back( &DefaultReporter );
        }

        // Select what to run.
        std::vector<FPlannedRun> Plan;
        FBenchmarkRunInfo Info{};
        for ( const FBenchmarkMetadata &Metadata : RegisteredBenchmarks )
        {
            if ( InOptions.bFilterByKind && Metadata.Kind != InOptions.KindFilter )
            {
                continue;
            }
            if ( not MatchesFilter( InOptions.SuiteFilter, Metadata.SuiteName, Metadata.BenchmarkName ) )
            {
                continue;
            }

            Info.bContainsUnoptimizedBenchmarks = Info.bContainsUnoptimizedBenchmarks || not Metadata.bOptimizedBuild;

            if ( Metadata.Arguments.empty() )
            {
                Plan.push_back( FPlannedRun{ &Metadata, false, 0 } );
            }
            else
            {
                for ( const Int64 Argument : Metadata.Arguments )
                {
                    Plan.push_back( FPlannedRun{ &Metadata, true, Argument } );
                }
            }
        }

        // Static initialization order across translation units is unspecified: sort for a deterministic report.
        std::stable_sort( Plan.begin(), Plan.end(),
                          [] ( const FPlannedRun &A, const FPlannedRun &B )
                          {
                              const std::string_view SuiteA = ToStdView( A.Metadata->SuiteName );
                              const std::string_view SuiteB = ToStdView( B.Metadata->SuiteName );
                              if ( SuiteA != SuiteB )
                              {
                                  return SuiteA < SuiteB;
                              }

                              const std::string_view FileA = A.Metadata->DeclaredLocation.file_name();
                              const std::string_view FileB = B.Metadata->DeclaredLocation.file_name();
                              if ( FileA != FileB )
                              {
                                  return FileA < FileB;
                              }

                              if ( A.Metadata->DeclaredLocation.line() != B.Metadata->DeclaredLocation.line() )
                              {
                                  return A.Metadata->DeclaredLocation.line() < B.Metadata->DeclaredLocation.line();
                              }
                              return A.Argument < B.Argument;
                          } );

        Info.BenchmarkCount = Plan.size();
        for ( const FPlannedRun &Run : Plan )
        {
            Info.LongestNameLength = std::max<USize>(
                Info.LongestNameLength,
                FormatBenchmarkName( Run.Metadata->SuiteName, Run.Metadata->BenchmarkName, Run.bHasArgument, Run.Argument )
                    .size() );
        }

        // Execute.
        FBenchmarkRunSummary Summary{};
        const FClock::time_point RunStart = FClock::now();

        for ( IBenchmarkReporter *Reporter : Reporters )
        {
            Reporter->OnRunStarted( Info );
        }

        for ( const FPlannedRun &Run : Plan )
        {
            const FBenchmarkResult Result = MeasureBenchmark( *Run.Metadata, Run.bHasArgument, Run.Argument, InOptions );

            ++Summary.TotalExecuted;
            if ( Result.bFailed )
            {
                ++Summary.FailedCount;
            }
            else
            {
                ++Summary.PassedCount;
            }

            for ( IBenchmarkReporter *Reporter : Reporters )
            {
                Reporter->OnBenchmarkFinished( Result );
            }
        }

        Summary.DurationSeconds = std::chrono::duration<Float64>( FClock::now() - RunStart ).count();

        for ( IBenchmarkReporter *Reporter : Reporters )
        {
            Reporter->OnRunFinished( Summary );
        }

        return Summary;
    }

} // namespace Benchmarks

} // namespace Lumen
