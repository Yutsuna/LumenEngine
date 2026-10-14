///
/// @file LaunchEngineLoop.cpp
/// @brief FLaunchEngineLoop implementation.
///

#include <Engine/LaunchEngineLoop.hpp>

#include <HAL/PlatformTime.hpp>
#include <HAL/Signal.hpp>

#include <print>
#include <thread>

namespace Lumen::Launch
{
namespace
{
    TAtomic<Bool> GExitRequested{ false };
    static_assert( TAtomic<Bool>::is_always_lock_free, "Lock-free atomics are required to be async-signal-safe." );

    /// @brief Handles signals that request engine exit.
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
    void OnOutOfMemory () noexcept
    {
        std::fputs( "[Lumen] Fatal: out of memory\n", stderr );
        std::_Exit( static_cast<Int32>( ELaunchErrorCode::OutOfMemory ) );
    }

    /// @brief Handles command-line options that specify numeric values.
    /// @tparam Member The member of FLaunchEngineConfig to set.
    /// @param InCommandLine The command-line to parse.
    /// @param InKey The command-line option key to look for.
    template <auto Member> [[nodiscard]] auto WithOption ( const FCommandLine &InCommandLine, const FStringView InKey ) noexcept
    {
        using FValue = std::remove_cvref_t<decltype( std::declval<FLaunchEngineConfig &>().*Member )>;

        return [&InCommandLine, InKey] ( FLaunchEngineConfig InConfig ) noexcept -> TLaunchResult<FLaunchEngineConfig>
        {
            return InCommandLine.GetNumber<FValue>( InKey )
                .Transform(
                    [&InConfig] ( const FValue InValue ) noexcept -> FLaunchEngineConfig
                    {
                        InConfig.*Member = InValue;
                        return InConfig;
                    } )
                .OrElse(
                    [&InConfig] ( const FLaunchError &InError ) noexcept -> TLaunchResult<FLaunchEngineConfig>
                    {
                        if ( InError.Code == ELaunchErrorCode::MissingArgument )
                        {
                            return InConfig;
                        }
                        return MakeUnexpected( InError );
                    } );
        };
    }

    /// @brief Reads `-Name` into `Config.*Member`.
    /// @tparam Member The member of FLaunchEngineConfig to set.
    /// @param InCommandLine The command-line to parse.
    /// @param InName The command-line option key to look for.
    template <auto Member> [[nodiscard]] auto WithSwitch ( const FCommandLine &InCommandLine, const FStringView InName ) noexcept
    {
        return [&InCommandLine, InName] ( FLaunchEngineConfig InConfig ) noexcept -> TLaunchResult<FLaunchEngineConfig>
        {
            InConfig.*Member = InConfig.*Member or InCommandLine.HasSwitch( InName );
            return InConfig;
        };
    }

    /// @brief Validates the configuration after parsing command-line options.
    /// @param InConfig The configuration to validate.
    [[nodiscard]] TLaunchResult<FLaunchEngineConfig> ValidateConfig ( FLaunchEngineConfig &&InConfig ) noexcept
    {
        if ( not InConfig.IsValid() ) [[unlikely]]
        {
            return LaunchFailure( ELaunchStage::PreInit, ELaunchErrorCode::InvalidConfiguration,
                                  "Invalid configuration (check -fixedstep, -maxframetime and -fps)" );
        }

        return InConfig;
    }

    /// @brief Parse the configuration with command-line options.
    /// @param InCommandLine The command-line to parse.
    [[nodiscard]] TLaunchResult<FLaunchEngineConfig> ParseConfig ( const FCommandLine &InCommandLine ) noexcept
    {
        return TLaunchResult<FLaunchEngineConfig>( FLaunchEngineConfig{} )
            .AndThen( WithOption<&FLaunchEngineConfig::FixedTimeStep>( InCommandLine, "fixedstep" ) )
            .AndThen( WithOption<&FLaunchEngineConfig::MaxFrameTime>( InCommandLine, "maxframetime" ) )
            .AndThen( WithOption<&FLaunchEngineConfig::MaxFPS>( InCommandLine, "fps" ) )
            .AndThen( WithOption<&FLaunchEngineConfig::MaxFrames>( InCommandLine, "maxframes" ) )
            .AndThen( WithSwitch<&FLaunchEngineConfig::bHeadless>( InCommandLine, "headless" ) )
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

    /// @return Return the exit code for a successful run.
    [[nodiscard]] constexpr Int32 ToExitCode ( const FRunSummary & ) noexcept
    {
        return EXIT_SUCCESS;
    }

    /// @brief Report a Fatal failure
    /// @param InError The error to report
    [[nodiscard]] TLaunchResult<Int32> ReportFatalError ( const FLaunchError &InError ) noexcept
    {
        std::println( stderr, "[Lumen] Fatal error during {}: {:.{}} (code {})", ToCString( InError.Stage ),
                      InError.Message.data(), static_cast<int>( InError.Message.size() ), InError.ToExitCode() );

        return TLaunchResult<Int32>( InError.ToExitCode() );
    }
} // namespace

Int32 FLaunchEngineLoop::GuardedMain ( const Int32 InArgc, const AnsiChar *const *InArgv ) noexcept
{
    return PreInit( FCommandLine( InArgc, InArgv ) )
        .AndThen( &FLaunchEngineLoop::Init )
        .AndThen( &FLaunchEngineLoop::Run )
        .Transform( &ToExitCode )
        .OrElse( &ReportFatalError )
        .ValueOr( EXIT_FAILURE );
}

TLaunchResult<FPreInitState> FLaunchEngineLoop::PreInit ( const FCommandLine InCommandLine ) noexcept
{
    std::set_new_handler( &OnOutOfMemory );

    FSignal::Bind( ESystemSignal::Interrupt, &OnSignal );
    FSignal::Bind( ESystemSignal::Terminate, &OnSignal );

    ( void )HAL::FPlatformTime::Seconds();

    return ParseConfig( InCommandLine )
        .Transform( [InCommandLine] ( FLaunchEngineConfig &&InConfig ) noexcept
                    { return FPreInitState{ InCommandLine, std::move( InConfig ) }; } );
}

TLaunchResult<FInitState> FLaunchEngineLoop::Init ( FPreInitState &&InPreInitState ) noexcept
{
    FInitState State;
    State.Config = std::move( InPreInitState.Config );

    if ( auto Started = State.Modules.StartAll( GetEngineModules(), State.Config ); not Started ) [[unlikely]]
    {
        return MakeUnexpected( Started.Error() );
    }

    return State;
}

TLaunchResult<FRunSummary> FLaunchEngineLoop::Run ( FInitState &&InInitState ) noexcept
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

void FLaunchEngineLoop::RequestExit () noexcept
{
    GExitRequested.store( true, std::memory_order_release );
}

Bool FLaunchEngineLoop::IsExitRequested () noexcept
{
    return GExitRequested.load( std::memory_order_acquire );
}
} // namespace Lumen::Launch
