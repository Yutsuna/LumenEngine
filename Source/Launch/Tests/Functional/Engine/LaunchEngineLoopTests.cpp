/**
 * @file LaunchEngineLoopTests.cpp
 * @brief Functional tests for FLaunchEngineLoop pipeline stages.
 */

#include <Engine/LaunchEngineLoop.hpp>

#include <CLI/CommandLine.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Private
 */

namespace
{

    struct FCommandLineHolder
    {
        TVector<const AnsiChar *> Args;
        Launch::FCommandLine CommandLine;
    };

    [[nodiscard]] FCommandLineHolder MakeCommandLine ( const std::initializer_list<const AnsiChar *> InArgs ) noexcept
    {
        FCommandLineHolder Holder;
        Holder.Args        = TVector<const AnsiChar *>( InArgs );
        Holder.CommandLine = Launch::FCommandLine( static_cast<Int32>( Holder.Args.size() ), Holder.Args.data() );
        return Holder;
    }

} // namespace

/**
 * Public
 */

LUMEN_TEST( FLaunchEngineLoop_PreInit, SucceedsWithDefaults )
{
    const auto Holder = MakeCommandLine( { "Lumen" } );
    const auto Result = Launch::FLaunchEngineLoop::PreInit( Holder.CommandLine );

    LUMEN_CHECK_HAS_VALUE( Result );
}

LUMEN_TEST( FLaunchEngineLoop_PreInit, FailsWithInvalidConfig )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-fixedstep=0" } );
    const auto Result = Launch::FLaunchEngineLoop::PreInit( Holder.CommandLine );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Stage, Launch::ELaunchStage::PreInit );
}

LUMEN_TEST( FLaunchEngineLoop_Init, SucceedsInHeadlessMode )
{
    const auto CommandLineHolder = MakeCommandLine( { "Lumen", "-headless" } );

    Launch::FPreInitState PreInitState{
        .CommandLine = CommandLineHolder.CommandLine,
        .Config      = Launch::FLaunchEngineConfig{},
    };
    PreInitState.Config.bHeadless = true;

    const auto Result = Launch::FLaunchEngineLoop::Init( std::move( PreInitState ) );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Result->Modules.Num(), 2ULL );
}

LUMEN_TEST( FLaunchEngineLoop_Run, RespectsMaxFrames )
{
    const auto CommandLineHolder = MakeCommandLine( { "Lumen", "-headless", "-maxframes=3" } );

    Launch::FPreInitState PreInitState{
        .CommandLine = CommandLineHolder.CommandLine,
        .Config      = Launch::FLaunchEngineConfig{},
    };
    PreInitState.Config.bHeadless    = true;
    PreInitState.Config.MaxFrames    = 3ULL;
    PreInitState.Config.MaxFrameTime = 0.1;

    Launch::FLaunchEngineLoop::ResetExitRequest();

    auto InitResult = Launch::FLaunchEngineLoop::Init( std::move( PreInitState ) );
    LUMEN_CHECK_HAS_VALUE( InitResult );

    const auto RunResult = Launch::FLaunchEngineLoop::Run( std::move( *InitResult ) );

    LUMEN_CHECK_HAS_VALUE( RunResult );
    LUMEN_CHECK_EQ( RunResult->FrameCount, 3ULL );
    LUMEN_CHECK_FALSE( RunResult->bInterrupted );
}

LUMEN_TEST( FLaunchEngineLoop_Run, RespectsExitRequest )
{
    const auto CommandLineHolder = MakeCommandLine( { "Lumen", "-headless" } );

    Launch::FPreInitState PreInitState{
        .CommandLine = CommandLineHolder.CommandLine,
        .Config      = Launch::FLaunchEngineConfig{},
    };
    PreInitState.Config.bHeadless    = true;
    PreInitState.Config.MaxFrameTime = 0.1;
    PreInitState.Config.MaxFPS       = 0.0;

    Launch::FLaunchEngineLoop::ResetExitRequest();

    auto InitResult = Launch::FLaunchEngineLoop::Init( std::move( PreInitState ) );
    LUMEN_CHECK_HAS_VALUE( InitResult );

    Launch::FLaunchEngineLoop::RequestExit();

    const auto RunResult = Launch::FLaunchEngineLoop::Run( std::move( *InitResult ) );

    LUMEN_CHECK_HAS_VALUE( RunResult );
    LUMEN_CHECK_EQ( RunResult->FrameCount, 0ULL );
    LUMEN_CHECK_TRUE( RunResult->bInterrupted );
}

LUMEN_TEST( FLaunchEngineLoop_RequestExit, IsIdempotent )
{
    Launch::FLaunchEngineLoop::ResetExitRequest();

    const Bool First  = Launch::FLaunchEngineLoop::RequestExit();
    const Bool Second = Launch::FLaunchEngineLoop::RequestExit();

    LUMEN_CHECK_FALSE( First );
    LUMEN_CHECK_TRUE( Second );
    LUMEN_CHECK_TRUE( Launch::FLaunchEngineLoop::IsExitRequested() );
}

LUMEN_TEST( FLaunchEngineLoop_IsExitRequested, ReflectsReset )
{
    Launch::FLaunchEngineLoop::ResetExitRequest();
    LUMEN_CHECK_FALSE( Launch::FLaunchEngineLoop::IsExitRequested() );

    Launch::FLaunchEngineLoop::RequestExit();
    LUMEN_CHECK_TRUE( Launch::FLaunchEngineLoop::IsExitRequested() );

    Launch::FLaunchEngineLoop::ResetExitRequest();
    LUMEN_CHECK_FALSE( Launch::FLaunchEngineLoop::IsExitRequested() );
}

} // namespace Lumen
