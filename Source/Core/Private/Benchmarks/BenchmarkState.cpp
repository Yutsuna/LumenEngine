/// @file BenchmarkState.cpp
/// @brief Benchmark state implementation.

#include <Benchmarks/BenchmarkState.hpp>

#include <HAL/PlatformTime.hpp>

#include <utility>

///
/// FIterator
///

Lumen::Benchmarks::FBenchmarkState::FIterator::FIterator ( FBenchmarkState &InState, const UInt64 InRemaining ) noexcept
    : State{ InState }, Remaining{ InRemaining }
{
    ///
}

Lumen::Benchmarks::FBenchmarkState::FIterator::~FIterator () noexcept
{
    State.EndLoop( Remaining );
}

Lumen::Benchmarks::FBenchmarkState::FIterationToken Lumen::Benchmarks::FBenchmarkState::FIterator::operator*() const noexcept
{
    return {};
}

Lumen::Benchmarks::FBenchmarkState::FIterator &Lumen::Benchmarks::FBenchmarkState::FIterator::operator++() noexcept
{
    --Remaining;
    return *this;
}

Lumen::Bool Lumen::Benchmarks::FBenchmarkState::FIterator::operator!=( std::default_sentinel_t ) const noexcept
{
    return Remaining != 0;
}

///
/// FBenchmarkState
///

Lumen::Benchmarks::FBenchmarkState::FBenchmarkState ( const UInt64 InIterations, TOptional<Int64> InArgument ) noexcept
    : Iterations{ InIterations }, Argument{ std::move( InArgument ) }
{
    ///
}

Lumen::Benchmarks::FBenchmarkState::FIterator
Lumen::Benchmarks::FBenchmarkState::begin () noexcept /// NOLINT(readability-identifier-naming)
{
    StartTimer();
    return FIterator{ *this, Iterations };
}

std::default_sentinel_t Lumen::Benchmarks::FBenchmarkState::end () const noexcept /// NOLINT(readability-identifier-naming)
{
    return {};
}

///
/// Timer
///

void Lumen::Benchmarks::FBenchmarkState::PauseTiming () noexcept
{
    StopTimer();
}

void Lumen::Benchmarks::FBenchmarkState::ResumeTiming () noexcept
{
    StartTimer();
}

void Lumen::Benchmarks::FBenchmarkState::SetItemsPerIteration ( const UInt64 InItems ) noexcept
{
    ItemsPerIteration = InItems;
}

void Lumen::Benchmarks::FBenchmarkState::SetBytesPerIteration ( const UInt64 InBytes ) noexcept
{
    BytesPerIteration = InBytes;
}

void Lumen::Benchmarks::FBenchmarkState::Finish () noexcept
{
    StopTimer();
}

///
/// Getters
///

Lumen::UInt64 Lumen::Benchmarks::FBenchmarkState::GetIterations () const noexcept
{
    return Iterations;
}

Lumen::UInt64 Lumen::Benchmarks::FBenchmarkState::GetExecutedIterations () const noexcept
{
    return Executed;
}

Int64 Lumen::Benchmarks::FBenchmarkState::GetArgument () const noexcept
{
    return Argument.ValueOr( 0 );
}

Lumen::Bool Lumen::Benchmarks::FBenchmarkState::HasArgument () const noexcept
{
    return Argument.HasValue();
}

Lumen::Float64 Lumen::Benchmarks::FBenchmarkState::GetElapsedSeconds () const noexcept
{
    return Accumulated;
}

Lumen::UInt64 Lumen::Benchmarks::FBenchmarkState::GetItemsPerIteration () const noexcept
{
    return ItemsPerIteration;
}

Lumen::UInt64 Lumen::Benchmarks::FBenchmarkState::GetBytesPerIteration () const noexcept
{
    return BytesPerIteration;
}

///
/// Private
///

void Lumen::Benchmarks::FBenchmarkState::StartTimer () noexcept
{
    if ( not bTimerRunning )
    {
        StartTime     = HAL::FPlatformTime::Seconds();
        bTimerRunning = true;
    }
}

void Lumen::Benchmarks::FBenchmarkState::StopTimer () noexcept
{
    if ( bTimerRunning )
    {
        Accumulated += HAL::FPlatformTime::Seconds() - StartTime;
        bTimerRunning = false;
    }
}

void Lumen::Benchmarks::FBenchmarkState::EndLoop ( const UInt64 InRemaining ) noexcept
{
    StopTimer();
    Executed = InRemaining <= Iterations ? Iterations - InRemaining : 0;
}
