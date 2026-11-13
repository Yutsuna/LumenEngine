/**
 * @file LaunchEngineConfigParserTests.cpp
 * @brief Functional tests for LaunchEngineConfig parsing and validation.
 */

#include <Engine/LaunchEngineConfigParser.hpp>

#include <CLI/CommandLine.hpp>
#include <HAL/PlatformTime.hpp>
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

LUMEN_TEST( ParseEngineConfig_Defaults, ReturnsDefaultConfig )
{
    const auto Holder = MakeCommandLine( { "Lumen" } );
    const auto Result = Launch::ParseEngineConfig( Holder.CommandLine );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Result->FixedStep, HAL::FPlatformTime::DefaultTickRate );
    LUMEN_CHECK_EQ( Result->MaxFrameTime, HAL::FPlatformTime::MaxTickRate );
    LUMEN_CHECK_EQ( Result->MaxFPS, 0.0 );
    LUMEN_CHECK_EQ( Result->MaxFrames, 0ULL );
    LUMEN_CHECK_FALSE( Result->bHeadless );
}

LUMEN_TEST( ParseEngineConfig_FixedStep, ParsesValue )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-fixedstep=0.01" } );
    const auto Result = Launch::ParseEngineConfig( Holder.CommandLine );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Result->FixedStep, 0.01 );
}

LUMEN_TEST( ParseEngineConfig_MaxFrameTime, ParsesValue )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-maxframetime=0.05" } );
    const auto Result = Launch::ParseEngineConfig( Holder.CommandLine );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Result->MaxFrameTime, 0.05 );
}

LUMEN_TEST( ParseEngineConfig_MaxFPS, ParsesValue )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-maxfps=144" } );
    const auto Result = Launch::ParseEngineConfig( Holder.CommandLine );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Result->MaxFPS, 144.0 );
}

LUMEN_TEST( ParseEngineConfig_MaxFrames, ParsesValue )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-maxframes=10" } );
    const auto Result = Launch::ParseEngineConfig( Holder.CommandLine );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Result->MaxFrames, 10ULL );
}

LUMEN_TEST( ParseEngineConfig_Headless, ParsesSwitch )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-headless" } );
    const auto Result = Launch::ParseEngineConfig( Holder.CommandLine );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( Result->bHeadless );
}

LUMEN_TEST( ParseEngineConfig_CaseInsensitive, AcceptsMixedCase )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-FixedStep=0.02", "-MAXFRAMES=5" } );
    const auto Result = Launch::ParseEngineConfig( Holder.CommandLine );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Result->FixedStep, 0.02 );
    LUMEN_CHECK_EQ( Result->MaxFrames, 5ULL );
}

LUMEN_TEST( ParseEngineConfig_UnknownOption, ReturnsInvalidArgument )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-unknown" } );
    const auto Result = Launch::ParseEngineConfig( Holder.CommandLine );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Stage, Launch::ELaunchStage::PreInit );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::InvalidArgument );
}

LUMEN_TEST( ParseEngineConfig_MalformedNumber, ReturnsInvalidArgument )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-fps=abc" } );
    const auto Result = Launch::ParseEngineConfig( Holder.CommandLine );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::InvalidArgument );
}

LUMEN_TEST( ParseEngineConfig_Help, ReturnsNoneErrorCode )
{
    const auto Holder = MakeCommandLine( { "Lumen", "-help" } );
    const auto Result = Launch::ParseEngineConfig( Holder.CommandLine );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::None );
}

LUMEN_TEST( ValidateEngineConfig_Valid, AcceptsDefaultConfig )
{
    const auto Result = Launch::ValidateEngineConfig( Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_VALUE( Result );
}

LUMEN_TEST( ValidateEngineConfig_FixedStep, RejectsNonPositive )
{
    Launch::FLaunchEngineConfig Config;
    Config.FixedStep = 0.0;

    const auto Result = Launch::ValidateEngineConfig( Config );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::InvalidConfiguration );
}

LUMEN_TEST( ValidateEngineConfig_MaxFrameTime, RejectsValueBelowFixedStep )
{
    Launch::FLaunchEngineConfig Config;
    Config.FixedStep    = 0.02;
    Config.MaxFrameTime = 0.01;

    const auto Result = Launch::ValidateEngineConfig( Config );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::InvalidConfiguration );
}

LUMEN_TEST( ValidateEngineConfig_MaxFrameTime, RejectsExcessiveStepRatio )
{
    Launch::FLaunchEngineConfig Config;
    Config.FixedStep    = 0.001;
    Config.MaxFrameTime = 2.0;

    const auto Result = Launch::ValidateEngineConfig( Config );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::InvalidConfiguration );
}

LUMEN_TEST( ValidateEngineConfig_MaxFPS, RejectsNegative )
{
    Launch::FLaunchEngineConfig Config;
    Config.MaxFPS = -1.0;

    const auto Result = Launch::ValidateEngineConfig( Config );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::InvalidConfiguration );
}

} // namespace Lumen
