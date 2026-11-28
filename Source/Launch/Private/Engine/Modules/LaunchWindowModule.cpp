/**
 * @file LaunchWindowModule.cpp
 * @brief FLaunchWindowModule implementation.
 */

#include <Engine/Modules/LaunchWindowModule.hpp>

#include <Generic/GenericApplication.hpp>
#include <Generic/GenericWindow.hpp>
#include <Generic/GenericWindowDescription.hpp>

#include <utility>

/**
 * Registration
 */

LUMEN_REGISTER_MODULE( Lumen::Launch::FLaunchWindowModule )

/**
 * Public
 */

Lumen::Bool Lumen::Launch::FLaunchWindowModule::IsEnabled ( const FLaunchEngineConfig &InConfig ) noexcept
{
    return not InConfig.bHeadless;
}

Lumen::Launch::TLaunchResult<Lumen::TUniquePtr<Lumen::Launch::FLaunchWindowModule>>
Lumen::Launch::FLaunchWindowModule::Create ( const TModuleContext<FLaunchWindowModule> &InContext ) noexcept
{
    const TSharedPtr<FGenericApplication> &Application = InContext.Get<FLaunchApplicationModule>().GetApplication();
    if ( not Application.IsValid() ) [[unlikely]]
    {
        return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                              "Application module holds an invalid application instance" );
    }

    /** TODO: expose -width/-height/-windowmode CLI options into FLaunchEngineConfig or module CliOptions. */
    const TSharedRef<FGenericWindow> Window                 = Application->MakeWindow();
    const TSharedRef<FGenericWindowDescription> Description = MakeShared<FGenericWindowDescription>();
    Description->GraphicsApi                                = EWindowGraphicsApi::OpenGL;

    Application->InitializeWindow( Window, Description, nullptr, true );

    return MakeUnique<FLaunchWindowModule>( TSharedPtr<FGenericWindow>( Window ) );
}

Lumen::Launch::FLaunchWindowModule::FLaunchWindowModule ( TSharedPtr<FGenericWindow> InWindow ) noexcept
    : Window( std::move( InWindow ) )
{
    /** Empty */
}

const Lumen::TSharedPtr<Lumen::FGenericWindow> &Lumen::Launch::FLaunchWindowModule::GetWindow () const noexcept
{
    return Window;
}
