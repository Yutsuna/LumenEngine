/**
 * @file LaunchEngineConfigTests.cpp
 * @brief Functional tests for FLaunchEngineConfig defaults and traits.
 */

#include <Engine/LaunchEngineConfig.hpp>

#include <HAL/PlatformTime.hpp>
#include <Tests/UnitTest.hpp>

#include <type_traits>

namespace Lumen
{

/**
 * Public
 */

LUMEN_TEST( FLaunchEngineConfig_Traits, IsTriviallyCopyable )
{
    static_assert( std::is_trivially_copyable_v<Launch::FLaunchEngineConfig> );
    LUMEN_CHECK_TRUE( std::is_trivially_copyable_v<Launch::FLaunchEngineConfig> );
}

LUMEN_TEST( FLaunchEngineConfig_Defaults, AreCoherent )
{
    const Launch::FLaunchEngineConfig Defaults;

    LUMEN_CHECK_EQ( Defaults.FixedStep, HAL::FPlatformTime::DefaultTickRate );
    LUMEN_CHECK_EQ( Defaults.MaxFrameTime, HAL::FPlatformTime::MaxTickRate );
    LUMEN_CHECK_EQ( Defaults.MaxFPS, 0.0 );
    LUMEN_CHECK_EQ( Defaults.MaxFrames, 0ULL );
    LUMEN_CHECK_FALSE( Defaults.bHeadless );
}

LUMEN_TEST( FLaunchEngineConfig_Copy, PreservesValues )
{
    Launch::FLaunchEngineConfig Config;
    Config.FixedStep    = 0.01;
    Config.MaxFrameTime = 0.1;
    Config.MaxFPS       = 30.0;
    Config.MaxFrames    = 100ULL;
    Config.bHeadless    = true;

    const Launch::FLaunchEngineConfig Copy = Config;

    LUMEN_CHECK_EQ( Copy.FixedStep, 0.01 );
    LUMEN_CHECK_EQ( Copy.MaxFrameTime, 0.1 );
    LUMEN_CHECK_EQ( Copy.MaxFPS, 30.0 );
    LUMEN_CHECK_EQ( Copy.MaxFrames, 100ULL );
    LUMEN_CHECK_TRUE( Copy.bHeadless );
}

} // namespace Lumen
