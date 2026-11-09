/**
 * @file LaunchWindowModule.cpp
 * @brief FLaunchWindowModule implementation.
 */

#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Engine/Modules/LaunchWindowModule.hpp>

#include <Generic/GenericApplication.hpp>
#include <Generic/GenericWindow.hpp>
#include <Generic/GenericWindowDescription.hpp>

Lumen::Launch::FLaunchWindowModule::FLaunchWindowModule ( Lumen::TSharedPtr<Lumen::FGenericWindow> InWindow ) noexcept
    : Window( std::move( InWindow ) )
{
    /** Empty */
}

Lumen::Launch::TLaunchResult<Lumen::TUniquePtr<Lumen::Launch::IEngineModule>>
Lumen::Launch::MakeWindowModule ( const FLaunchEngineConfig &InConfig ) noexcept
{
    if ( InConfig.bHeadless )
    {
        return MakeUnique<FLaunchWindowModule>( nullptr );
    }

    const TSharedPtr<FGenericApplication> Application = GetApplication();
    if ( not Application.IsValid() )
    {
        return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                              "Window module requires the Application module to be started first" );
    }

    /** TODO: expose -width/-height/-windowmode CLI options into FLaunchEngineConfig. */
    const TSharedRef<FGenericWindow> Window                 = Application->MakeWindow();
    const TSharedRef<FGenericWindowDescription> Description = MakeShared<FGenericWindowDescription>();

    Application->InitializeWindow( Window, Description, nullptr, true );

    return MakeUnique<FLaunchWindowModule>( TSharedPtr<FGenericWindow>( Window ) );
}

Lumen::FStringView Lumen::Launch::FLaunchWindowModule::GetName () const noexcept
{
    return "Window";
}
