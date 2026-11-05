/// @file BenchmarkState.hpp
/// @brief Benchmark state definition.

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>
#include <CoreExport.hpp>

#include <Templates/Optional.hpp>

#include <iterator>

namespace Lumen
{

namespace Benchmarks
{

    ///
    /// @class FBenchmarkState
    /// @brief Controls the measured loop of a benchmark.
    ///
    /// @code
    /// for ( auto _ : State )
    /// {
    ///     // measured body
    /// }
    /// @endcode
    ///
    class CORE_EXPORT FBenchmarkState final : public FNonCopyable, public FNonMovable
    {
    public:

        /// @brief Dummy value yielded by the loop (`for ( auto _ : State )`).
        struct FIterationToken
        {
        };

        ///
        /// @class FIterator
        /// @brief Loop iterator. Stops the timer when the loop ends, breaks, or throws.
        ///
        class CORE_EXPORT FIterator final : public FNonCopyable, public FNonMovable
        {
        public:

            FIterator ( FBenchmarkState &InState, UInt64 InRemaining ) noexcept;
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
        explicit FBenchmarkState ( UInt64 InIterations, TOptional<Int64> InArgument = {} ) noexcept;

    public:

        [[nodiscard]] FIterator begin () noexcept;                   /// NOLINT(readability-identifier-naming)
        [[nodiscard]] std::default_sentinel_t end () const noexcept; /// NOLINT(readability-identifier-naming)

    public:

        /// @brief Stops the clock.
        void PauseTiming () noexcept;

        /// @brief Restarts the clock after PauseTiming().
        void ResumeTiming () noexcept;

        /// @brief Declares how many logical items one iteration processes.
        void SetItemsPerIteration ( UInt64 InItems ) noexcept;

        /// @brief Declares how many bytes one iteration processes.
        void SetBytesPerIteration ( UInt64 InBytes ) noexcept;

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
        void EndLoop ( UInt64 InRemaining ) noexcept;

    private:

        UInt64 Iterations;
        UInt64 Executed{ 0 };
        TOptional<Int64> Argument;

        UInt64 ItemsPerIteration{ 0 };
        UInt64 BytesPerIteration{ 0 };

        Float64 StartTime{ 0.0 };
        Float64 Accumulated{ 0.0 };
        Bool bTimerRunning{ false };
    };

} // namespace Benchmarks

} // namespace Lumen
