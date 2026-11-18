/**
 * @file LaunchEngineLoopTests.cpp
 * @brief Functional tests for FLaunchEngineLoop pipeline stages.
 */

#include <Engine/LaunchEngineLoop.hpp>

#include <CLI/CommandLine.hpp>
#include <Engine/LaunchEngineModule.hpp>
#include <HAL/PlatformTime.hpp>
#include <Memory/UniquePtr.hpp>
#include <Templates/Vector.hpp>
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

    UInt32 GTickCallCount = 0U;

    class FTickProbeModule final : public Launch::IEngineModule
    {
    public:

        [[nodiscard]] FStringView GetName () const noexcept override
        {
            return "TickProbe";
        }

        [[nodiscard]] Launch::EModuleTraits GetTraits () const noexcept override
        {
            return Launch::EModuleTraits::Tick;
        }

        void Tick ( const Float64, const Float64 ) noexcept override
        {
            ++GTickCallCount;
        }
    };

    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeTickProbeModule ( const Launch::FLaunchEngineConfig & ) noexcept
    {
        return MakeUnique<FTickProbeModule>();
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
    LUMEN_CHECK_TRUE( Result->CommandLine.GetProgramName() == "Lumen" );
    LUMEN_CHECK_EQ( Result->Config.FixedStep, HAL::FPlatformTime::DefaultTickRate );
    LUMEN_CHECK_EQ( Result->Config.MaxFrameTime, HAL::FPlatformTime::MaxTickRate );
    LUMEN_CHECK_EQ( Result->Config.MaxFPS, 0.0 );
    LUMEN_CHECK_EQ( Result->Config.MaxFrames, 0ULL );
    LUMEN_CHECK_FALSE( Result->Config.bHeadless );
}

LUMEN_TEST( FLaunchEngineLoop_PreInit, AppliesCommandLineOptions )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-fixedstep=0.02", "-headless" } );
    const auto Result = Launch::FLaunchEngineLoop::PreInit( Holder.CommandLine );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Result->Config.FixedStep, 0.02 );
    LUMEN_CHECK_TRUE( Result->Config.bHeadless );
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
    LUMEN_CHECK_TRUE( RunResult->ElapsedSeconds >= 0.0 );
}

LUMEN_TEST( FLaunchEngineLoop_Run, TicksModulesOncePerFrame )
{
    Launch::FInitState State;
    State.Config.bHeadless    = true;
    State.Config.MaxFrames    = 2ULL;
    State.Config.MaxFrameTime = 0.1;

    static constexpr Launch::FModuleDescriptor Descriptors[] = {
        Launch::FModuleDescriptor{ .Name = "TickProbe", .Create = &MakeTickProbeModule },
    };

    const auto Started = State.Modules.StartAll( Descriptors, State.Config );
    LUMEN_CHECK_HAS_VALUE( Started );

    GTickCallCount = 0U;

    Launch::FLaunchEngineLoop::ResetExitRequest();

    const auto RunResult = Launch::FLaunchEngineLoop::Run( std::move( State ) );

    LUMEN_CHECK_HAS_VALUE( RunResult );
    LUMEN_CHECK_EQ( RunResult->FrameCount, 2ULL );
    LUMEN_CHECK_EQ( GTickCallCount, 2U );
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
