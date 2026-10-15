///
/// @file LaunchEngineLoop.cpp
/// @brief Launch::FLaunchEngineLoop implementation.
///

#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/LaunchEngineConfigParser.hpp>
#include <Engine/LaunchFrameTiming.hpp>

#include <Logging/Logger.hpp>

#include <HAL/PlatformTime.hpp>

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

    FFrameClock Clock( Config.FixedTimeStep, Config.MaxFrameTime );
    FFrameLimiter Limiter( Config.MaxFPS );
    UInt64 FrameCount = 0;

    while ( not IsExitRequested() ) [[likely]]
    {
        const auto [DeltaSeconds, FixedSteps, Alpha] = Clock.BeginFrame();

        for ( UInt32 Index = 0; Index < FixedSteps; ++Index )
        {
            State.Modules.FixedTick( Config.FixedTimeStep );
        }
        State.Modules.Tick( DeltaSeconds, Alpha );

        ++FrameCount;
        if ( Config.MaxFrames != 0 and FrameCount >= Config.MaxFrames ) [[unlikely]]
        {
            break;
        }

        Limiter.Wait();
    }

    return FRunSummary{
        .FrameCount     = FrameCount,
        .ElapsedSeconds = Clock.ElapsedSeconds(),
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