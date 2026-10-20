///
/// @file LaunchFrameTiming.cpp
/// @brief FFrameClock / FFrameLimiter implementation.
///

#include <Engine/LaunchFrameTiming.hpp>

#include <HAL/PlatformTime.hpp>

#include <algorithm>
#include <thread>

namespace Lumen
{

namespace
{

    /// @brief Sleep until the specified deadline.
    /// @param InDeadline The deadline to sleep until.
    /// @return The current time in seconds.
    [[nodiscard]] Float64 WaitUntil ( const Float64 InDeadline ) noexcept
    {
        static constexpr Float64 SpinMargin = 0.0015;
        Float64 NowSeconds                  = HAL::FPlatformTime::Seconds();

        if ( const Float64 Remaining = InDeadline - NowSeconds; Remaining > SpinMargin )
        {
            HAL::FPlatformTime::Sleep( Remaining - SpinMargin );
            NowSeconds = HAL::FPlatformTime::Seconds();
        }

        while ( NowSeconds < InDeadline )
        {
            std::this_thread::yield();
            NowSeconds = HAL::FPlatformTime::Seconds();
        }
        return NowSeconds;
    }

} // namespace

Launch::FFrameClock::FFrameClock ( const Float64 InFixedTimeStep, const Float64 InMaxFrameTime ) noexcept
    : FFrameClock( InFixedTimeStep, InMaxFrameTime, HAL::FPlatformTime::Seconds() )
{
    /// Empty
}

Launch::FFrameClock::FFrameClock ( const Float64 InFixedTimeStep,
                                   const Float64 InMaxFrameTime,
                                   const Float64 InStartSeconds ) noexcept
    : FixedTimeStep( InFixedTimeStep ), MaxFrameTime( InMaxFrameTime ), StartTime( InStartSeconds ),
      PreviousTime( InStartSeconds )
{
    /// Empty
}

Launch::FFrameStep Launch::FFrameClock::BeginFrame () noexcept
{
    return BeginFrame( HAL::FPlatformTime::Seconds() );
}

Launch::FFrameStep Launch::FFrameClock::BeginFrame ( const Float64 InNowSeconds ) noexcept
{
    const Float64 Delta = std::clamp( InNowSeconds - PreviousTime, 0.0, MaxFrameTime );

    PreviousTime = InNowSeconds;
    Accumulator += Delta;

    UInt32 FixedSteps = 0;
    while ( Accumulator >= FixedTimeStep )
    {
        Accumulator -= FixedTimeStep;
        ++FixedSteps;
    }

    return FFrameStep{ .DeltaSeconds = Delta, .FixedSteps = FixedSteps, .Alpha = Accumulator / FixedTimeStep };
}

Float64 Launch::FFrameClock::ElapsedSeconds () const noexcept
{
    return HAL::FPlatformTime::Seconds() - StartTime;
}

Launch::FFrameLimiter::FFrameLimiter ( const Float64 InMaxFPS ) noexcept
    : FrameBudget( InMaxFPS > 0.0 ? 1.0 / InMaxFPS : 0.0 ), NextDeadline( HAL::FPlatformTime::Seconds() )
{
    /// Empty
}

void Launch::FFrameLimiter::Wait () noexcept
{
    if ( FrameBudget <= 0.0 )
    {
        return;
    }

    NextDeadline += FrameBudget;

    if ( const Float64 WokeAt = WaitUntil( NextDeadline ); WokeAt - NextDeadline > FrameBudget ) [[unlikely]]
    {
        NextDeadline = WokeAt;
    }
}

} // namespace Lumen
