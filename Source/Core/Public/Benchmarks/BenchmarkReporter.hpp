/**
 * @file BenchmarkReporter.hpp
 * @brief Reporter interface and built-in reporters (console table, JSON file, debug dump).
 */

#pragma once

#include <CoreExport.hpp>

#include <Benchmarks/BenchmarkTypes.hpp>

#include <Templates/Vector.hpp>

namespace Lumen
{

namespace Benchmarks
{

    /**
     * @class IBenchmarkReporter
     * @brief Receives the events emitted by a benchmark run.
     */
    class CORE_EXPORT IBenchmarkReporter
    {
    public:

        virtual ~IBenchmarkReporter () noexcept = default;

        virtual void OnRunStarted ( const FBenchmarkRunInfo &Info )         = 0;
        virtual void OnBenchmarkFinished ( const FBenchmarkResult &Result ) = 0;
        virtual void OnRunFinished ( const FBenchmarkRunSummary &Summary )  = 0;
    };

    /**
     * @class FConsoleBenchmarkReporter
     * @brief Prints an aligned table on stdout, one row per finished benchmark.
     */
    class CORE_EXPORT FConsoleBenchmarkReporter final : public IBenchmarkReporter
    {
    public:

        void OnRunStarted ( const FBenchmarkRunInfo &Info ) override;
        void OnBenchmarkFinished ( const FBenchmarkResult &Result ) override;
        void OnRunFinished ( const FBenchmarkRunSummary &Summary ) override;

    private:

        USize NameColumnWidth{ 40 };
    };

    /**
     * @class FJsonBenchmarkReporter
     * @brief Writes every result to a JSON file when the run ends.
     */
    class CORE_EXPORT FJsonBenchmarkReporter final : public IBenchmarkReporter
    {
    public:

        explicit FJsonBenchmarkReporter ( FString InOutputPath );

        void OnRunStarted ( const FBenchmarkRunInfo &Info ) override;
        void OnBenchmarkFinished ( const FBenchmarkResult &Result ) override;
        void OnRunFinished ( const FBenchmarkRunSummary &Summary ) override;

    private:

        FString OutputPath;
        TVector<FBenchmarkResult> Results;
        Bool bOptimized{ true };
    };

    /**
     * @class FDebugBenchmarkReporter
     * @brief Dumps every event as JSON via Meta::SerializeToJSON. Debug/CI only.
     */
    class CORE_EXPORT FDebugBenchmarkReporter final : public IBenchmarkReporter
    {
    public:

        void OnRunStarted ( const FBenchmarkRunInfo &Info ) override;
        void OnBenchmarkFinished ( const FBenchmarkResult &Result ) override;
        void OnRunFinished ( const FBenchmarkRunSummary &Summary ) override;
    };

} // namespace Benchmarks

} // namespace Lumen
