/**
 * @file LaunchApplicationModule.cpp
 * @brief FLaunchApplicationModule implementation.
 */

#include <Core/CoreDefinitions.hpp>

#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>

#include <Generic/GenericApplication.hpp>

#if defined( LUMEN_PLATFORM_LINUX )
    #include <Linux/LinuxApplication.hpp>
#endif

#include <utility>

/**
 * Registration
 */

LUMEN_REGISTER_MODULE( Lumen::Launch::FLaunchApplicationModule )

/**
 * FLaunchMessageHandler
 */

void Lumen::Launch::FLaunchMessageHandler::OnRequestExit () noexcept
{
    FLaunchEngineLoop::RequestExit();
}

void Lumen::Launch::FLaunchMessageHandler::OnQuit () noexcept
{
    FLaunchEngineLoop::RequestExit();
}

/**
 * FLaunchApplicationModule
 */

Lumen::Bool Lumen::Launch::FLaunchApplicationModule::IsEnabled ( const FLaunchEngineConfig &InConfig ) noexcept
{
    return not InConfig.bHeadless;
}

Lumen::Launch::TLaunchResult<Lumen::TUniquePtr<Lumen::Launch::FLaunchApplicationModule>>
Lumen::Launch::FLaunchApplicationModule::Create ( const TModuleContext<FLaunchApplicationModule> LUMEN_UNUSED &InContext ) noexcept
{
#if defined( LUMEN_PLATFORM_LINUX )
    const TSharedPtr<FGenericApplication> PlatformApplication = FLinuxApplication::MakeLinuxApplication();
    if ( not PlatformApplication.IsValid() ) [[unlikely]]
    {
        return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                              "Failed to create platform application" );
    }

    return MakeUnique<FLaunchApplicationModule>( PlatformApplication );
#else
    return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                          "No platform application implementation for this target" );
#endif
}

Lumen::Launch::FLaunchApplicationModule::FLaunchApplicationModule ( TSharedPtr<FGenericApplication> InApplication ) noexcept
    : Application( std::move( InApplication ) ), MessageHandler( MakeShared<FLaunchMessageHandler>() )
{
    if ( Application.IsValid() ) [[likely]]
    {
        Application->SetMessageHandler( MessageHandler );
    }
}

Lumen::Launch::FLaunchApplicationModule::~FLaunchApplicationModule () noexcept
{
    if ( Application.IsValid() ) [[likely]]
    {
        Application->Shutdown();
        Application.Reset();
    }
}

void Lumen::Launch::FLaunchApplicationModule::Tick ( const Float64 InDeltaSeconds, const Float64 LUMEN_UNUSED InAlpha ) noexcept
{
    if ( Application.IsValid() ) [[likely]]
    {
        Application->PumpMessages( InDeltaSeconds );
    }
}

Lumen::Launch::EModuleTraits Lumen::Launch::FLaunchApplicationModule::GetTraits () const noexcept
{
    return EModuleTraits::Tick;
}

const Lumen::TSharedPtr<Lumen::FGenericApplication> &Lumen::Launch::FLaunchApplicationModule::GetApplication () const noexcept
{
    return Application;
}
