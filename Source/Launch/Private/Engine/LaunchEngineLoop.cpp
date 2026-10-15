///
/// @file LaunchEngineLoop.cpp
/// @brief Launch::FLaunchEngineLoop implementation.
///

#include <Engine/LaunchEngineLoop.hpp>

#include <Engine/LaunchEngineConfigParser.hpp>

#include <Logging/Logger.hpp>

#include <HAL/PlatformTime.hpp>
#include <HAL/Signal.hpp>

#include <algorithm>
#include <memory>
#include <thread>

namespace Lumen
{

namespace
{
    TAtomic<Bool> GExitRequested{ false };
    static_assert( TAtomic<Bool>::is_always_lock_free, "Lock-free atomics are required to be async-signal-safe." );

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

Launch::TLaunchResult<Launch::FPreInitState> Launch::FLaunchEngineLoop::PreInit ( const FCommandLine &InCommandLine ) noexcept
{
    /// INFO: Initialize the clock epoch now so that no later measurement pays for its lazy initialization.
    ( void )HAL::FPlatformTime::Seconds();

    return ParseEngineConfig( InCommandLine )
        .Transform( [&InCommandLine] ( FLaunchEngineConfig &&InConfig ) noexcept -> FPreInitState
                    { return FPreInitState{ .CommandLine = InCommandLine, .Config = std::move( InConfig ) }; } );
}

Launch::TLaunchResult<Launch::FInitState> Launch::FLaunchEngineLoop::Init ( FPreInitState &&InPreInitState ) noexcept
{
    FInitState State;
    State.Config = InPreInitState.Config;

    if ( TLaunchResult<void> Started = State.Modules.StartAll( GetEngineModules(), State.Config ); not Started ) [[unlikely]]
    {
        return MakeUnexpected( Started.Error() );
    }

    return State;
}

Launch::TLaunchResult<Launch::FRunSummary> Launch::FLaunchEngineLoop::Run ( FInitState &&InInitState ) noexcept
{
    FInitState State = std::move( InInitState );

    const FLaunchEngineConfig &Config = State.Config;
    const Float64 FrameBudget         = Config.MaxFPS > 0.0 ? 1.0 / Config.MaxFPS : 0.0;
    const Float64 StartTime           = HAL::FPlatformTime::Seconds();

    Float64 PreviousTime = StartTime;
    Float64 NextDeadline = StartTime;
    Float64 Accumulator  = 0.0;
    UInt64 FrameCount    = 0;

    while ( not IsExitRequested() ) [[likely]]
    {
        const Float64 FrameStart = HAL::FPlatformTime::Seconds();
        const Float64 Delta      = std::clamp( FrameStart - PreviousTime, 0.0, Config.MaxFrameTime );

        PreviousTime = FrameStart;
        Accumulator += Delta;

        while ( Accumulator >= Config.FixedTimeStep )
        {
            State.Modules.FixedTick( Config.FixedTimeStep );
            Accumulator -= Config.FixedTimeStep;
        }

        State.Modules.Tick( Delta, Accumulator / Config.FixedTimeStep );

        ++FrameCount;
        if ( Config.MaxFrames != 0 and FrameCount >= Config.MaxFrames ) [[unlikely]]
        {
            break;
        }

        if ( FrameBudget > 0.0 )
        {
            NextDeadline += FrameBudget;

            if ( const Float64 WokeAt = WaitUntil( NextDeadline ); WokeAt - NextDeadline > FrameBudget ) [[unlikely]]
            {
                NextDeadline = WokeAt;
            }
        }
    }

    return FRunSummary{
        .FrameCount     = FrameCount,
        .ElapsedSeconds = HAL::FPlatformTime::Seconds() - StartTime,
        .bInterrupted   = IsExitRequested(),
    };
}

Bool Launch::FLaunchEngineLoop::RequestExit () noexcept
{
    return GExitRequested.exchange( true, std::memory_order_acq_rel );
}

Bool Launch::FLaunchEngineLoop::IsExitRequested () noexcept
{
    return GExitRequested.load( std::memory_order_acquire );
}

} // namespace Lumen