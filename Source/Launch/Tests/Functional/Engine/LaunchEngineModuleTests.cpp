/**
 * @file LaunchEngineModuleTests.cpp
 * @brief Functional tests for engine module contract and descriptor registration.
 */

#include <Engine/LaunchEngineModule.hpp>

#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Engine/Modules/LaunchWindowModule.hpp>
#include <Launch/LaunchModuleStack.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Private
 */

namespace
{

    constexpr FStringView ApplicationModuleName = "FLaunchApplicationModule";
    constexpr FStringView WindowModuleName      = "FLaunchWindowModule";

    class FDefaultModule final : public Launch::IEngineModule
    {
    };

    /** @brief Descriptor names come from reflection; match against either the short or qualified spelling. */
    [[nodiscard]] Bool HasModuleName ( const Launch::FModuleDescriptor &InDescriptor, const FStringView InName ) noexcept
    {
        return InDescriptor.Name.find( InName ) != FStringView::npos or
               InDescriptor.QualifiedName.find( InName ) != FStringView::npos;
    }

    [[nodiscard]] const Launch::FModuleDescriptor *FindRegisteredModule ( const FStringView InName ) noexcept
    {
        for ( const Launch::FModuleDescriptor *const Descriptor : Launch::CollectRegisteredModules() )
        {
            if ( Descriptor != nullptr and HasModuleName( *Descriptor, InName ) )
            {
                return Descriptor;
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
        LUMEN_CHECK_TRUE( Module.GetName() == "Unknown" );

        Module.FixedTick( 0.016 );
        Module.Tick( 0.016, 0.5 );

        LUMEN_CHECK_TRUE( true );
    }

    LUMEN_TEST( GetRegisteredModules, ContainsApplicationAndWindow )
    {
        const auto Modules = Launch::CollectRegisteredModules();

        LUMEN_CHECK_TRUE( Modules.size() >= 2ULL );

        const Launch::FModuleDescriptor *const Application = FindRegisteredModule( ApplicationModuleName );
        const Launch::FModuleDescriptor *const Window      = FindRegisteredModule( WindowModuleName );

        LUMEN_CHECK_TRUE( Application != nullptr );
        LUMEN_CHECK_TRUE( Window != nullptr );
        LUMEN_CHECK_TRUE( Application->Create != nullptr );
        LUMEN_CHECK_TRUE( Application->IsEnabled != nullptr );
        LUMEN_CHECK_TRUE( Window->Create != nullptr );
        LUMEN_CHECK_TRUE( Window->IsEnabled != nullptr );
    }

    LUMEN_TEST( MakeApplicationModule_Headless, IsDisabled )
    {
        const Launch::FLaunchEngineConfig Config{ .bHeadless = true };
        const Launch::FCommandLine CommandLine;

        const Launch::FModuleDescriptor *const Descriptor = FindRegisteredModule( ApplicationModuleName );
        LUMEN_CHECK_TRUE( Descriptor != nullptr );
        LUMEN_CHECK_FALSE( Descriptor->IsEnabled( Config, CommandLine ) );
    }

    LUMEN_TEST( MakeWindowModule_Headless, IsDisabled )
    {
        const Launch::FLaunchEngineConfig Config{ .bHeadless = true };
        const Launch::FCommandLine CommandLine;

        const Launch::FModuleDescriptor *const Descriptor = FindRegisteredModule( WindowModuleName );
        LUMEN_CHECK_TRUE( Descriptor != nullptr );
        LUMEN_CHECK_FALSE( Descriptor->IsEnabled( Config, CommandLine ) );
    }

    LUMEN_TEST( MakeWindowModule_NotHeadless, DeclaresApplicationRequirement )
    {
        const Launch::FLaunchEngineConfig Config;
        const Launch::FCommandLine CommandLine;

        const Launch::FModuleDescriptor *const Application = FindRegisteredModule( ApplicationModuleName );
        const Launch::FModuleDescriptor *const Window      = FindRegisteredModule( WindowModuleName );

        LUMEN_CHECK_TRUE( Application != nullptr );
        LUMEN_CHECK_TRUE( Window != nullptr );
        LUMEN_CHECK_TRUE( Application->IsEnabled( Config, CommandLine ) );
        LUMEN_CHECK_TRUE( Window->IsEnabled( Config, CommandLine ) );
        LUMEN_CHECK_EQ( Window->Requires.size(), 1ULL );
        LUMEN_CHECK_TRUE( Window->Requires[0].Id == Application->Id );
        LUMEN_CHECK_TRUE( Window->Requires[0].Name == ApplicationModuleName );
    }

    LUMEN_TEST( GetApplication_Headless, NoApplicationServiceRegistered )
    {
        const Launch::FLaunchEngineConfig Config{ .bHeadless = true };

        Launch::FLaunchModuleStack Stack;
        const auto Started = Stack.StartAll( Config );

        LUMEN_CHECK_HAS_VALUE( Started );
        LUMEN_CHECK_EQ( Stack.Num(), 0ULL );
        LUMEN_CHECK_TRUE( Stack.FindService<Launch::FLaunchApplicationModule>() == nullptr );
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

        Launch::FLaunchEngineLoop::ResetExitRequest();
        Handler.OnKeyDown( EKeys::R, false );
        LUMEN_CHECK_FALSE( Launch::FLaunchEngineLoop::IsExitRequested() );
        Handler.OnKeyDown( EKeys::Escape, false );
        LUMEN_CHECK_TRUE( Launch::FLaunchEngineLoop::IsExitRequested() );
        Launch::FLaunchEngineLoop::ResetExitRequest();
    }

} // namespace

} // namespace Lumen
