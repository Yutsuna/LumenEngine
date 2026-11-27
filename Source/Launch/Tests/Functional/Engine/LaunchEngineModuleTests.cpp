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
    };

    /** @brief Finds a registered factory by the traits of the module it creates. */
    [[nodiscard]] Launch::FModuleFactory FindFactory ( const Launch::EModuleTraits InTraits ) noexcept
    {
        const Launch::FLaunchEngineConfig Config{ .bHeadless = true };

        for ( const Launch::FModuleFactory &Factory : Launch::GetEngineModules() )
        {
            auto Created = Factory( Config );
            if ( Created and ( *Created )->GetTraits() == InTraits )
            {
                return Factory;
            }
        }
        return nullptr;
    }

    /**
     * Public
     */

    LUMEN_TEST( IEngineModule_Defaults, HasNoTraitsAndNoOpHooks )
    {
        FDefaultModule Module;

        LUMEN_CHECK_EQ( Module.GetTraits(), Launch::EModuleTraits::None );
        LUMEN_CHECK_TRUE( Module.GetName() == "IEngineModule" );

        Module.FixedTick( 0.016 );
        Module.Tick( 0.016, 0.5 );

        LUMEN_CHECK_TRUE( true );
    }

    LUMEN_TEST( GetEngineModules, ContainsApplicationAndWindow )
    {
        const auto Modules = Launch::GetEngineModules();

        LUMEN_CHECK_EQ( Modules.size(), 2ULL );
        LUMEN_CHECK_TRUE( Modules[0] != nullptr );
        LUMEN_CHECK_TRUE( Modules[1] != nullptr );
    }

    LUMEN_TEST( MakeApplicationModule_Headless, ReturnsValidModule )
    {
        Launch::FLaunchEngineConfig Config;
        Config.bHeadless = true;

        const Launch::FModuleFactory Factory = FindFactory( Launch::EModuleTraits::Tick );
        LUMEN_CHECK_TRUE( Factory != nullptr );
        const auto Result = Factory( Config );

        LUMEN_CHECK_HAS_VALUE( Result );
        LUMEN_CHECK_TRUE( static_cast<Bool>( *Result ) );
        LUMEN_CHECK_TRUE( ( *Result )->GetName() == "IEngineModule" );
        LUMEN_CHECK_TRUE( HasAnyFlags( ( *Result )->GetTraits(), Launch::EModuleTraits::Tick ) );
        LUMEN_CHECK_FALSE( HasAnyFlags( ( *Result )->GetTraits(), Launch::EModuleTraits::FixedTick ) );

        /** INFO: Ticking without a platform application must be a safe no-op. */
        ( *Result )->Tick( 0.016, 0.5 );
    }

    LUMEN_TEST( MakeWindowModule_Headless, ReturnsValidModule )
    {
        Launch::FLaunchEngineConfig Config;
        Config.bHeadless = true;

        const Launch::FModuleFactory Factory = FindFactory( Launch::EModuleTraits::None );
        LUMEN_CHECK_TRUE( Factory != nullptr );
        const auto Result = Factory( Config );

        LUMEN_CHECK_HAS_VALUE( Result );
        LUMEN_CHECK_TRUE( static_cast<Bool>( *Result ) );
        LUMEN_CHECK_TRUE( ( *Result )->GetName() == "IEngineModule" );
        LUMEN_CHECK_EQ( ( *Result )->GetTraits(), Launch::EModuleTraits::None );
    }

    LUMEN_TEST( MakeWindowModule_NotHeadless, FailsWithoutApplication )
    {
        /** NOTE: Relies on no earlier test having created a platform application (only headless ones are). */
        LUMEN_CHECK_FALSE( Launch::GetApplication().IsValid() );

        const Launch::FLaunchEngineConfig Config;

        const Launch::FModuleFactory Factory = FindFactory( Launch::EModuleTraits::None );
        LUMEN_CHECK_TRUE( Factory != nullptr );
        const auto Result = Factory( Config );

        LUMEN_CHECK_HAS_ERROR( Result );
        LUMEN_CHECK_EQ( Result.Error().Stage, Launch::ELaunchStage::Init );
        LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::ModuleCreationFailed );
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

} // namespace

} // namespace Lumen
