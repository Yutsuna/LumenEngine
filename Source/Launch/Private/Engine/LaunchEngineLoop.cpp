///
/// @file LaunchEngineLoop.cpp
/// @brief Launch::FLaunchEngineLoop implementation.
///

#include <Engine/LaunchEngineLoop.hpp>

#include "Core/CoreDefinitions.hpp"

#include <HAL/PlatformTime.hpp>
#include <HAL/Signal.hpp>
#include <Logging/Logger.hpp>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <new>
#include <thread>

namespace Lumen
{

namespace
{
    TAtomic<Bool> GExitRequested{ false };
    static_assert( TAtomic<Bool>::is_always_lock_free, "Lock-free atomics are required to be async-signal-safe." );

    /// @brief Handles signals that request engine exit.
    /// @note Must stay async-signal-safe: never log from here.
    /// @param InSignal The signal that triggered the exit request.
    void OnSignal ( const ESystemSignal::Type InSignal ) noexcept
    {
        if ( GExitRequested.exchange( true, std::memory_order_acq_rel ) )
        {
            FSignal::Unbind( InSignal );
            FSignal::Raise( InSignal );
        }
    }

    /// @brief Handles out-of-memory conditions.
    /// @note LUMEN_LOG allocates (formatting, queue), which would re-enter this handler: use the raw Flush instead.
    void OnOutOfMemory () noexcept
    {
        FLogger::Flush( "[Lumen] Fatal: out of memory\n" );
        std::_Exit( static_cast<Int32>( Launch::ELaunchErrorCode::OutOfMemory ) );
    }

    /// @brief Handles command-line options that specify numeric values.
    /// @tparam Member The member of Launch::FLaunchEngineConfig to set.
    /// @param InCommandLine The command-line to parse.
    /// @param InKey The command-line option key to look for.
    template <auto Member>
    [[nodiscard]] auto WithOption ( const Launch::FCommandLine &InCommandLine, const FStringView InKey ) noexcept
    {
        using FValue = std::remove_cvref_t<decltype( std::declval<Launch::FLaunchEngineConfig &>().*Member )>;

        return [&InCommandLine,
                InKey] ( Launch::FLaunchEngineConfig InConfig ) noexcept -> Launch::TLaunchResult<Launch::FLaunchEngineConfig>
        {
            return InCommandLine.GetNumber<FValue>( InKey )
                .Transform(
                    [&InConfig] ( const FValue InValue ) noexcept -> Launch::FLaunchEngineConfig
                    {
                        InConfig.*Member = InValue;
                        return InConfig;
                    } )
                .OrElse(
                    [&InConfig] (
                        const Launch::FLaunchError &InError ) noexcept -> Launch::TLaunchResult<Launch::FLaunchEngineConfig>
                    {
                        if ( InError.Code == Launch::ELaunchErrorCode::MissingArgument )
                        {
                            return InConfig;
                        }
                        return MakeUnexpected( InError );
                    } );
        };
    }

    /// @brief Reads `-Name` into `Config.*Member`.
    /// @tparam Member The member of Launch::FLaunchEngineConfig to set.
    /// @param InCommandLine The command-line to parse.
    /// @param InName The command-line option key to look for.
    template <auto Member>
    [[nodiscard]] auto WithSwitch ( const Launch::FCommandLine &InCommandLine, const FStringView InName ) noexcept
    {
        return [&InCommandLine,
                InName] ( Launch::FLaunchEngineConfig InConfig ) noexcept -> Launch::TLaunchResult<Launch::FLaunchEngineConfig>
        {
            InConfig.*Member = InConfig.*Member or InCommandLine.HasSwitch( InName );
            return InConfig;
        };
    }

    /// @brief Validates the configuration after parsing command-line options.
    /// @param InConfig The configuration to validate.
    [[nodiscard]] Launch::TLaunchResult<Launch::FLaunchEngineConfig>
    ValidateConfig ( Launch::FLaunchEngineConfig &&InConfig ) noexcept
    {
        if ( not InConfig.IsValid() ) [[unlikely]]
        {
            return LaunchFailure( Launch::ELaunchStage::PreInit, Launch::ELaunchErrorCode::InvalidConfiguration,
                                  "Invalid configuration (check -fixedstep, -maxframetime and -fps)" );
        }

        return InConfig;
    }

    /// @brief Parse the configuration with command-line options.
    /// @param InCommandLine The command-line to parse.
    [[nodiscard]] Launch::TLaunchResult<Launch::FLaunchEngineConfig>
    ParseConfig ( const Launch::FCommandLine &InCommandLine ) noexcept
    {
        return Launch::TLaunchResult<Launch::FLaunchEngineConfig>( Launch::FLaunchEngineConfig{} )
            .AndThen( WithOption<&Launch::FLaunchEngineConfig::FixedTimeStep>( InCommandLine, "fixedstep" ) )
            .AndThen( WithOption<&Launch::FLaunchEngineConfig::MaxFrameTime>( InCommandLine, "maxframetime" ) )
            .AndThen( WithOption<&Launch::FLaunchEngineConfig::MaxFPS>( InCommandLine, "fps" ) )
            .AndThen( WithOption<&Launch::FLaunchEngineConfig::MaxFrames>( InCommandLine, "maxframes" ) )
            .AndThen( WithSwitch<&Launch::FLaunchEngineConfig::bHeadless>( InCommandLine, "headless" ) )
            .AndThen( ValidateConfig );
    }

    /// @brief Sleep until the specified deadline.
    /// @param InDeadline The deadline to sleep until.
    void WaitUntil ( const Float64 InDeadline ) noexcept
    {
        static constexpr Float64 SpinMargin = 0.0015;

        if ( const Float64 Remaining = InDeadline - HAL::FPlatformTime::Seconds(); Remaining > SpinMargin )
        {
            HAL::FPlatformTime::Sleep( Remaining - SpinMargin );
        }

        while ( HAL::FPlatformTime::Seconds() < InDeadline )
        {
            std::this_thread::yield();
        }
    }

} // namespace

Launch::TLaunchResult<Launch::FPreInitState> Launch::FLaunchEngineLoop::PreInit ( const FCommandLine &InCommandLine ) noexcept
{
    std::set_new_handler( &OnOutOfMemory );

    FSignal::Bind( ESystemSignal::Interrupt, &OnSignal );
    FSignal::Bind( ESystemSignal::Terminate, &OnSignal );

    static const struct FRaiiGuardScope
    {
        FRaiiGuardScope () noexcept
        {
            FLogger::GetInstance().Initialize();
        }
        ~FRaiiGuardScope () noexcept
        {
            FLogger::GetInstance().Shutdown();
        }
    } RaiiGuardedScope;

    ( void )HAL::FPlatformTime::Seconds();

    return ParseConfig( InCommandLine )
        .Transform( [InCommandLine] ( FLaunchEngineConfig &&InConfig ) noexcept -> FPreInitState
                    { return FPreInitState{ .CommandLine = InCommandLine, .Config = std::move( InConfig ) }; } );
}

Launch::TLaunchResult<Launch::FInitState> Launch::FLaunchEngineLoop::Init ( FPreInitState &&InPreInitState ) noexcept
{
    FInitState State;
    State.Config = std::move( InPreInitState.Config );

    if ( auto Started = State.Modules.StartAll( GetEngineModules(), State.Config ); not Started ) [[unlikely]]
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
    Float64 Accumulator  = 0.0;
    UInt64 FrameCount    = 0;

    while ( not IsExitRequested() ) [[likely]]
    {
        const Float64 FrameStart = HAL::FPlatformTime::Seconds();
        const Float64 Delta      = std::min( FrameStart - PreviousTime, Config.MaxFrameTime );

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
            WaitUntil( FrameStart + FrameBudget );
        }
    }

    return FRunSummary{
        .FrameCount     = FrameCount,
        .ElapsedSeconds = HAL::FPlatformTime::Seconds() - StartTime,
        .bInterrupted   = IsExitRequested(),
    };
}

void Launch::FLaunchEngineLoop::RequestExit () noexcept
{
    GExitRequested.store( true, std::memory_order_release );
}

Bool Launch::FLaunchEngineLoop::IsExitRequested () noexcept
{
    return GExitRequested.load( std::memory_order_acquire );
}

} // namespace Lumen