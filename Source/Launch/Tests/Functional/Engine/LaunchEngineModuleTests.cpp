/**
 * @file LaunchEngineModuleTests.cpp
 * @brief Functional tests for engine module contract and registration.
 */

#include <Engine/LaunchEngineModule.hpp>

#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Engine/Modules/LaunchWindowModule.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Private
 */

namespace
{

    class FDefaultModule final : public Launch::IEngineModule
    {
    public:

        [[nodiscard]] FStringView GetName () const noexcept override
        {
            return "Default";
        }
    };

} // namespace

/**
 * Public
 */

LUMEN_TEST( IEngineModule_Defaults, HasNoTraitsAndNoOpHooks )
{
    FDefaultModule Module;

    LUMEN_CHECK_EQ( Module.GetTraits(), Launch::EModuleTraits::None );
    LUMEN_CHECK_TRUE( Module.GetName() == "Default" );

    Module.FixedTick( 0.016 );
    Module.Tick( 0.016, 0.5 );

    LUMEN_CHECK_TRUE( true );
}

LUMEN_TEST( GetEngineModules, ContainsApplicationAndWindow )
{
    const auto Modules = Launch::GetEngineModules();

    LUMEN_CHECK_EQ( Modules.size(), 2ULL );
    LUMEN_CHECK_TRUE( Modules[0].Name == "Application" );
    LUMEN_CHECK_TRUE( Modules[0].Create != nullptr );
    LUMEN_CHECK_TRUE( Modules[1].Name == "Window" );
    LUMEN_CHECK_TRUE( Modules[1].Create != nullptr );
}

LUMEN_TEST( MakeApplicationModule_Headless, ReturnsValidModule )
{
    Launch::FLaunchEngineConfig Config;
    Config.bHeadless = true;

    const auto Result = Launch::MakeApplicationModule( Config );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( static_cast<Bool>( *Result ) );
    LUMEN_CHECK_TRUE( ( *Result )->GetName() == "Application" );
}

LUMEN_TEST( MakeWindowModule_Headless, ReturnsValidModule )
{
    Launch::FLaunchEngineConfig Config;
    Config.bHeadless = true;

    const auto Result = Launch::MakeWindowModule( Config );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( static_cast<Bool>( *Result ) );
    LUMEN_CHECK_TRUE( ( *Result )->GetName() == "Window" );
}

LUMEN_TEST( GetApplication_Headless, ReturnsNull )
{
    LUMEN_CHECK_FALSE( Launch::GetApplication().IsValid() );
}

LUMEN_TEST( FLaunchMessageHandler, RequestsExitOnEvents )
{
    Launch::FLaunchEngineLoop::ResetExitRequest();
    LUMEN_CHECK_FALSE( Launch::FLaunchEngineLoop::IsExitRequested() );

    Launch::FLaunchMessageHandler Handler;
    Handler.OnRequestExit();

    LUMEN_CHECK_TRUE( Launch::FLaunchEngineLoop::IsExitRequested() );

    Launch::FLaunchEngineLoop::ResetExitRequest();
    Handler.OnQuit();

    LUMEN_CHECK_TRUE( Launch::FLaunchEngineLoop::IsExitRequested() );
}

} // namespace Lumen
