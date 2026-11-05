/// @file BenchmarkReporter.hpp
/// @brief Reporter interface and built-in reporters (console table, JSON file, debug dump).

#pragma once

#include <CoreExport.hpp>

#include <Benchmarks/BenchmarkTypes.hpp>

#include <string>
#include <vector>

namespace Lumen
{

namespace Benchmarks
{

    class IBenchmarkReporter
    {
    public:

        virtual ~IBenchmarkReporter () = default;

        virtual void OnRunStarted ( const FBenchmarkRunInfo &Info )         = 0;
        virtual void OnBenchmarkFinished ( const FBenchmarkResult &Result ) = 0;
        virtual void OnRunFinished ( const FBenchmarkRunSummary &Summary )  = 0;
    };

    /// @brief Prints an aligned table on stdout, one row per finished benchmark.
    class CORE_EXPORT FConsoleBenchmarkReporter final : public IBenchmarkReporter
    {
    public:

        void OnRunStarted ( const FBenchmarkRunInfo &Info ) override;
        void OnBenchmarkFinished ( const FBenchmarkResult &Result ) override;
        void OnRunFinished ( const FBenchmarkRunSummary &Summary ) override;

    private:

        USize NameColumnWidth{ 40 };
    };

    /// @brief Writes every result to a JSON file when the run ends.
    /// @details Schema is hand-written (not Meta::SerializeToJSON) to stay stable across
    ///          field reorderings and to omit non-applicable keys (argument, throughput).
    class CORE_EXPORT FJsonBenchmarkReporter final : public IBenchmarkReporter
    {
    public:

        explicit FJsonBenchmarkReporter ( std::string InOutputPath );

        void OnRunStarted ( const FBenchmarkRunInfo &Info ) override;
        void OnBenchmarkFinished ( const FBenchmarkResult &Result ) override;
        void OnRunFinished ( const FBenchmarkRunSummary &Summary ) override;

    private:

        std::string OutputPath;
        std::vector<FBenchmarkResult> Results;
        Bool bOptimized{ true };
    };

    /// @brief Dumps every event as JSON via Meta::SerializeToJSON. Debug/CI only.
    class CORE_EXPORT FDebugBenchmarkReporter final : public IBenchmarkReporter
    {
    public:

        void OnRunStarted ( const FBenchmarkRunInfo &Info ) override;
        void OnBenchmarkFinished ( const FBenchmarkResult &Result ) override;
        void OnRunFinished ( const FBenchmarkRunSummary &Summary ) override;
    };

} // namespace Benchmarks

} // namespace Lumen
