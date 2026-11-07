/**
 * @file Benchmark.cpp
 * @brief Benchmark registry, calibration and statistics.
 */

#include <Benchmarks/Benchmark.hpp>

#include <Benchmarks/BenchmarkReporter.hpp>

#include <HAL/PlatformTime.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <format>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace Lumen
{

namespace Benchmarks
{

    namespace
    {

        [[nodiscard]] Bool WildcardMatch ( const FStringView Pattern, const FStringView Text ) noexcept
        {
            constexpr USize NoStar = static_cast<USize>( -1 );

            USize P     = 0;
            USize T     = 0;
            USize StarP = NoStar;
            USize StarT = 0;

            while ( T < Text.size() )
            {
                if ( P < Pattern.size() and ( Pattern[P] == '?' or Pattern[P] == Text[T] ) )
                {
                    ++P;
                    ++T;
                }
                else if ( P < Pattern.size() and Pattern[P] == '*' )
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

            while ( P < Pattern.size() and Pattern[P] == '*' )
            {
                ++P;
            }
            return P == Pattern.size();
        }

        [[nodiscard]] Bool MatchesFilter ( const FStringView InFilter, const FStringView Suite, const FStringView Name )
        {
            if ( InFilter.empty() )
            {
                return true;
            }

            const FString FullName = std::format( "{}.{}", Suite, Name );
            if ( InFilter.find_first_of( "*?" ) != FStringView::npos )
            {
                return WildcardMatch( InFilter, Suite ) or WildcardMatch( InFilter, FullName );
            }
            return FullName.find( InFilter ) != FString::npos;
        }

        struct FMeasurement
        {
            Float64 ElapsedSeconds{ 0.0 };
            UInt64 Executed{ 0 };
            UInt64 ItemsPerIteration{ 0 };
            UInt64 BytesPerIteration{ 0 };
        };

        [[nodiscard]] FMeasurement
        RunOnce ( const FBenchmarkMetadata &Metadata, const UInt64 Iterations, const TOptional<Int64> &Argument )
        {
            FBenchmarkState State{ Iterations, Argument };
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
                                                          const TOptional<Int64> &Argument,
                                                          const FBenchmarkRunOptions &Options )
        {
            FBenchmarkResult Result{
                .SuiteName     = FString{ Metadata.SuiteName },
                .BenchmarkName = FString{ Metadata.BenchmarkName },
                .Kind          = Metadata.Kind,
                .Argument      = Argument,
                .Failure       = {},
            };

            FBenchmarkConfig Config = FBenchmarkConfig::MakeDefault( Metadata.Kind );
            if ( Options.RepetitionsOverride.HasValue() )
            {
                Config.Repetitions = *Options.RepetitionsOverride;
            }
            if ( Options.MinTimeOverrideSeconds.HasValue() )
            {
                Config.MinTimeSeconds = *Options.MinTimeOverrideSeconds;
            }
            Config.Repetitions = Config.Repetitions > 0 ? Config.Repetitions : 1;

            try
            {
                // Pick the iteration count. For calibrated benchmarks this doubles as the warm-up.
                UInt64 Iterations = Config.FixedIterations > 0 ? Config.FixedIterations : 1;
                if ( Config.MinTimeSeconds > 0.0 )
                {
                    Iterations = 1;
                    for ( ;; )
                    {
                        const FMeasurement Probe = RunOnce( Metadata, Iterations, Argument );
                        if ( Probe.ElapsedSeconds >= Config.MinTimeSeconds or Iterations >= Config.MaxIterations )
                        {
                            break;
                        }

                        const Float64 PerIteration = Probe.ElapsedSeconds / static_cast<Float64>( Probe.Executed );
                        const Float64 Current      = static_cast<Float64>( Iterations );
                        const Float64 Wanted  = PerIteration > 0.0 ? Config.MinTimeSeconds * 1.4 / PerIteration : Current * 10.0;
                        const Float64 Clamped = std::clamp( Wanted, Current * 2.0, Current * 10.0 );

                        Iterations = std::min<UInt64>( static_cast<UInt64>( std::ceil( Clamped ) ), Config.MaxIterations );
                    }
                }

                for ( USize Run = 0; Run < Config.WarmupRuns; ++Run )
                {
                    static_cast<void>( RunOnce( Metadata, Iterations, Argument ) );
                }

                TVector<Float64> SamplesNs;
                SamplesNs.reserve( Config.Repetitions );

                UInt64 ItemsPerIteration = 0;
                UInt64 BytesPerIteration = 0;
                for ( USize Run = 0; Run < Config.Repetitions; ++Run )
                {
                    const FMeasurement Sample = RunOnce( Metadata, Iterations, Argument );
                    SamplesNs.push_back( Sample.ElapsedSeconds * 1.0E9 / static_cast<Float64>( Sample.Executed ) );

                    ItemsPerIteration = Sample.ItemsPerIteration;
                    BytesPerIteration = Sample.BytesPerIteration;
                }

                Result.IterationsPerSample     = Iterations;
                Result.SampleCount             = SamplesNs.size();
                Result.NanosecondsPerIteration = ComputeStatistics( std::move( SamplesNs ) );

                const Float64 MedianNs = Result.NanosecondsPerIteration.Median;
                if ( MedianNs > 0.0 )
                {
                    Result.ItemsPerSecond = static_cast<Float64>( ItemsPerIteration ) * 1.0E9 / MedianNs;
                    Result.BytesPerSecond = static_cast<Float64>( BytesPerIteration ) * 1.0E9 / MedianNs;
                }
            }
            catch ( const std::exception &Exception )
            {
                Result.Failure.Emplace( Exception.what() );
            }
            catch ( ... )
            {
                Result.Failure.Emplace( "unknown exception" );
            }

            return Result;
        }

        struct FPlannedRun
        {
            const FBenchmarkMetadata *Metadata{ nullptr };
            TOptional<Int64> Argument;
        };

    } // namespace

    FBenchmarkStatistics ComputeStatistics ( TVector<Float64> Samples )
    {
        FBenchmarkStatistics Stats{};
        if ( Samples.empty() )
        {
            return Stats;
        }

        std::ranges::sort( Samples );

        const USize Count = Samples.size();
        Stats.Min         = Samples.front();
        Stats.Max         = Samples.back();
        Stats.Median      = ( Count % 2 == 1 ) ? Samples[Count / 2] : ( Samples[( Count / 2 ) - 1] + Samples[Count / 2] ) * 0.5;
        Stats.Mean        = std::accumulate( Samples.begin(), Samples.end(), 0.0 ) / static_cast<Float64>( Count );

        if ( Count > 1 )
        {
            Float64 SquaredDeviations = 0.0;
            for ( const Float64 Sample : Samples )
            {
                const Float64 Difference = Sample - Stats.Mean;
                SquaredDeviations += Difference * Difference;
            }
            Stats.StdDev = std::sqrt( SquaredDeviations / static_cast<Float64>( Count - 1 ) );
        }

        Stats.CoefficientOfVariation = Stats.Mean > 0.0 ? Stats.StdDev / Stats.Mean * 100.0 : 0.0;
        return Stats;
    }

    FString FormatBenchmarkName ( const FStringView SuiteName, const FStringView BenchmarkName, const TOptional<Int64> Argument )
    {
        FString Name = std::format( "{}.{}", SuiteName, BenchmarkName );
        if ( Argument.HasValue() )
        {
            Name += std::format( "/{}", *Argument );
        }
        return Name;
    }

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
        TVector<IBenchmarkReporter *> Reporters = InOptions.Reporters;
        if ( Reporters.empty() )
        {
            Reporters.push_back( &DefaultReporter );
        }

        TVector<const FBenchmarkMetadata *> SelectedBenchmarks;
        FBenchmarkRunInfo Info{};
        for ( const FBenchmarkMetadata &Metadata : RegisteredBenchmarks )
        {
            if ( InOptions.KindFilter.HasValue() and Metadata.Kind != *InOptions.KindFilter )
            {
                continue;
            }
            if ( not MatchesFilter( InOptions.SuiteFilter, Metadata.SuiteName, Metadata.BenchmarkName ) )
            {
                continue;
            }

            SelectedBenchmarks.push_back( &Metadata );
            Info.bContainsUnoptimizedBenchmarks = Info.bContainsUnoptimizedBenchmarks or not Metadata.bOptimizedBuild;
        }

        // Static initialization order across translation units is unspecified: sort for a deterministic report.
        std::ranges::stable_sort( SelectedBenchmarks,
                                  [] ( const FBenchmarkMetadata *A, const FBenchmarkMetadata *B )
                                  {
                                      if ( A->SuiteName != B->SuiteName )
                                      {
                                          return A->SuiteName < B->SuiteName;
                                      }
                                      if ( A->BenchmarkName != B->BenchmarkName )
                                      {
                                          return A->BenchmarkName < B->BenchmarkName;
                                      }

                                      const FStringView FileA = A->DeclaredLocation.file_name();
                                      const FStringView FileB = B->DeclaredLocation.file_name();
                                      if ( FileA != FileB )
                                      {
                                          return FileA < FileB;
                                      }
                                      return A->DeclaredLocation.line() < B->DeclaredLocation.line();
                                  } );

        TVector<FPlannedRun> Plan;
        for ( const FBenchmarkMetadata *Metadata : SelectedBenchmarks )
        {
            if ( Metadata->Arguments.empty() )
            {
                Plan.push_back( FPlannedRun{ .Metadata = Metadata, .Argument = TOptional<Int64>{} } );
                continue;
            }

            for ( const Int64 Argument : Metadata->Arguments )
            {
                Plan.push_back( FPlannedRun{ .Metadata = Metadata, .Argument = TOptional<Int64>{ Argument } } );
            }
        }

        Info.BenchmarkCount = Plan.size();
        for ( const FPlannedRun &Run : Plan )
        {
            Info.LongestNameLength =
                std::max( Info.LongestNameLength,
                          FormatBenchmarkName( Run.Metadata->SuiteName, Run.Metadata->BenchmarkName, Run.Argument ).size() );
        }

        FBenchmarkRunSummary Summary{};
        const Float64 RunStart = HAL::FPlatformTime::Seconds();

        for ( IBenchmarkReporter *Reporter : Reporters )
        {
            Reporter->OnRunStarted( Info );
        }

        for ( const FPlannedRun &Run : Plan )
        {
            const FBenchmarkResult Result = MeasureBenchmark( *Run.Metadata, Run.Argument, InOptions );

            ++Summary.TotalExecuted;
            if ( Result.Succeeded() )
            {
                ++Summary.PassedCount;
            }
            else
            {
                ++Summary.FailedCount;
            }

            for ( IBenchmarkReporter *Reporter : Reporters )
            {
                Reporter->OnBenchmarkFinished( Result );
            }
        }

        Summary.DurationSeconds = HAL::FPlatformTime::Seconds() - RunStart;

        for ( IBenchmarkReporter *Reporter : Reporters )
        {
            Reporter->OnRunFinished( Summary );
        }

        return Summary;
    }

} // namespace Benchmarks

} // namespace Lumen
