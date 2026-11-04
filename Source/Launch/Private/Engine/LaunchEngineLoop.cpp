///
/// @file LaunchEngineLoop.cpp
/// @brief Launch::FLaunchEngineLoop implementation.
///

#include "Engine/LaunchEngineConfig.hpp"
#include <Engine/LaunchEngineConfigParser.hpp>
#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/LaunchFrameTiming.hpp>

#include <Meta/Serialize.hpp>

#include <Logging/Logger.hpp>

#include <HAL/PlatformTime.hpp>

namespace Lumen
{

namespace
{

    TAtomic<Bool> GExitRequested{ false };
    static_assert( TAtomic<Bool>::is_always_lock_free, "Lock-free atomics are required to be async-signal-safe." );

    LUMEN_LOG_DEFINE_CATEGORY( LogEngineLoop, "EngineLoop" );

    void
    RunEngineState ( Launch::FInitState &State, const Launch::FLaunchEngineConfig &Config, Launch::FFrameClock &Clock ) noexcept
    {
        const auto [DeltaSeconds, FixedSteps, Alpha] = Clock.BeginFrame();

        for ( UInt32 Index = 0; Index < FixedSteps; ++Index )
        {
            State.Modules.FixedTick( Config.FixedTimeStep );
        }
        State.Modules.Tick( DeltaSeconds, Alpha );
    }

} // namespace

Launch::TLaunchResult<Launch::FPreInitState> Launch::FLaunchEngineLoop::PreInit ( const FCommandLine &InCommandLine ) noexcept
{
    /// INFO: Initialize the clock epoch now so that no later measurement pays for its lazy initialization.
    ( void )HAL::FPlatformTime::Seconds();

    LUMEN_LOG_VERBOSE( LogEngineLoop, "Starting '{}' ({} arguments)", InCommandLine.GetProgramName(),
                       InCommandLine.GetArguments().size() );

    return ParseEngineConfig( InCommandLine )
        .Transform(
            [&InCommandLine] ( FLaunchEngineConfig &&InConfig ) noexcept -> FPreInitState
            {
                const FPreInitState PreInitState = { .CommandLine = InCommandLine, .Config = std::move( InConfig ) };

                LUMEN_LOG_DISPLAY( LogEngineLoop, "Launching engine with config:" LUMEN_NEWLINE "{}",
                                   Meta::SerializeToString( InConfig ) );

                return PreInitState;
            } );
}

Launch::TLaunchResult<Launch::FInitState> Launch::FLaunchEngineLoop::Init ( FPreInitState &&InPreInitState ) noexcept
{
    FInitState State;
    State.Config = InPreInitState.Config;

    LUMEN_LOG_INFO( LogEngineLoop, "Initializing engine modules..." );

    if ( TLaunchResult<void> Started = State.Modules.StartAll( GetEngineModules(), State.Config ); not Started ) [[unlikely]]
    {
        return MakeUnexpected( Started.Error() );
    }

    LUMEN_LOG_INFO( LogEngineLoop, "{} module(s) started", State.Modules.Num() );

    return State;
}

Launch::TLaunchResult<Launch::FRunSummary> Launch::FLaunchEngineLoop::Run ( FInitState &&InInitState ) noexcept
{
    FInitState State = std::move( InInitState );

    const FLaunchEngineConfig &Config = State.Config;

    LUMEN_LOG_DISPLAY( LogEngineLoop, "Entering main loop..." );

    FFrameClock Clock( Config.FixedTimeStep, Config.MaxFrameTime );
    FFrameLimiter Limiter( Config.MaxFPS );
    UInt64 FrameCount = 0;

    while ( not IsExitRequested() ) [[likely]]
    {
        RunEngineState( State, Config, Clock );

        ++FrameCount;
        if ( Config.MaxFrames != 0 and FrameCount >= Config.MaxFrames ) [[unlikely]]
        {
            break;
        }

        Limiter.Wait();
    }

    const FRunSummary RunSummary{
        .FrameCount     = FrameCount,
        .ElapsedSeconds = Clock.ElapsedSeconds(),
        .bInterrupted   = IsExitRequested(),
    };

    LUMEN_LOG_INFO( LogEngineLoop, "Main loop exited." );
    LUMEN_LOG_DISPLAY( LogEngineLoop, "Run summary:" LUMEN_NEWLINE "{}", Meta::SerializeToString( RunSummary ) );
    return RunSummary;
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
