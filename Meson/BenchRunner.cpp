/*
- @file BenchRunner.cpp
- @brief Standard entry-point for Lumen module benchmark suites.
 */

#include <Benchmarks/Benchmarks.hpp>

#include <Logging/Logger.hpp>

Lumen::Int32 main ( const Lumen::Int32 Argc, const Lumen::AnsiChar *Argv[] )
{
    Lumen::Benchmarks::FBenchmarkRunOptions Options{};
    if ( Argc > 1 )
    {
        Options.SuiteFilter = Argv[1];
    }

    Lumen::Benchmarks::FConsoleBenchmarkReporter ConsoleReporter{};
    Lumen::Benchmarks::FJsonBenchmarkReporter JSonReporter{ Argc > 2 ? Argv[2] : "" };

    Options.Reporters.push_back( &ConsoleReporter );
    if ( Argc > 2 )
    {
        Options.Reporters.push_back( &JSonReporter );
    }

    struct FGuardedScope final
    {
        FGuardedScope () noexcept
        {
            Lumen::FLogger ::GetInstance().Initialize();
        }
        ~FGuardedScope () noexcept
        {
            Lumen::FLogger::GetInstance().Shutdown();
        }
    } GuardedScope{};

    return Lumen::Benchmarks::FBenchmarkRegistry::GetInstance().Run( Options ).IsSuccessful() ? 0 : 84;
}
