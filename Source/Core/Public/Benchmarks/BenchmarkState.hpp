/// @file BenchmarkState.hpp
/// @brief Benchmark state definition.

#pragma once

#include "Core/NonCopyable.hpp"
#include <Core/CoreTypes.hpp>

#include <chrono>
#include <iterator>

namespace Lumen
{

namespace Benchmarks
{

    ///
    /// @class FBenchmarkState
    /// @brief Controls the measured loop of a benchmark.
    ///
    class FBenchmarkState final : public FNonCopyable
    {
    public:

        using FClock = std::chrono::steady_clock;

        /// @brief Dummy value yielded by the loop (`for ( auto _ : State )`).
        struct [[maybe_unused]] FIterationToken
        {
        };

        /// @class FIterator
        /// @brief Loop iterator. Stops the timer when the loop ends, breaks, or throws.
        class FIterator final : public FNonCopyable
        {
        public:

            FIterator ( FBenchmarkState &InState, const UInt64 InRemaining ) noexcept;
            ~FIterator () noexcept;

            [[nodiscard]] FIterationToken operator*() const noexcept;
            FIterator &operator++() noexcept;
            [[nodiscard]] Bool operator!=( std::default_sentinel_t ) const noexcept;

        private:

            FBenchmarkState &State;
            UInt64 Remaining;
        };

    public:

        /// @note Constructed by the benchmark runner, not by user code.
        explicit FBenchmarkState ( const UInt64 InIterations,
                                   const Bool bInHasArgument = false,
                                   const Int64 InArgument    = 0 ) noexcept;

    public:

        [[nodiscard]] FIterator begin () noexcept;                   /// NOLINT(readability-identifier-naming)
        [[nodiscard]] std::default_sentinel_t end () const noexcept; /// NOLINT(readability-identifier-naming)

    public:

        /// @brief Stops the clock.
        void PauseTiming () noexcept;

        /// @brief Restarts the clock after PauseTiming().
        void ResumeTiming () noexcept;

        /// @brief Declares how many logical items one iteration processes.
        void SetItemsPerIteration ( const UInt64 InItems ) noexcept;

        /// @brief Declares how many bytes one iteration processes.
        void SetBytesPerIteration ( const UInt64 InBytes ) noexcept;

        /// @brief Called by the runner once the body returned.
        void Finish () noexcept;

    public:

        [[nodiscard]] UInt64 GetIterations () const noexcept;
        [[nodiscard]] UInt64 GetExecutedIterations () const noexcept;
        [[nodiscard]] Int64 GetArgument () const noexcept;
        [[nodiscard]] Bool HasArgument () const noexcept;
        [[nodiscard]] Float64 GetElapsedSeconds () const noexcept;
        [[nodiscard]] UInt64 GetItemsPerIteration () const noexcept;
        [[nodiscard]] UInt64 GetBytesPerIteration () const noexcept;

    private:

        void StartTimer () noexcept;
        void StopTimer () noexcept;
        void EndLoop ( const UInt64 InRemaining ) noexcept;

    private:

        UInt64 Iterations;
        UInt64 Executed{ 0 };
        Int64 Argument;
        Bool bHasArgument;

        UInt64 ItemsPerIteration{ 0 };
        UInt64 BytesPerIteration{ 0 };

        FClock::time_point StartPoint;
        FClock::duration Accumulated{ FClock::duration::zero() };
        Bool bTimerRunning{ false };
    };

} // namespace Benchmarks

} // namespace Lumen
