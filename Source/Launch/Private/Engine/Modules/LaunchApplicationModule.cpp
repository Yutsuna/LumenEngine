///
/// @file LaunchApplicationModule.cpp
/// @brief FLaunchApplicationModule implementation.
///

#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Generic/GenericApplication.hpp>

#if defined( LUMEN_PLATFORM_LINUX )
    #include <Linux/LinuxApplication.hpp>
#endif

namespace
{
Lumen::TSharedPtr<Lumen::FGenericApplication> GApplication = nullptr;
}

///
/// FLaunchMessageHandler
///

void Lumen::Launch::FLaunchMessageHandler::OnRequestExit () noexcept
{
    FLaunchEngineLoop::RequestExit();
}

void Lumen::Launch::FLaunchMessageHandler::OnQuit () noexcept
{
    FLaunchEngineLoop::RequestExit();
}

///
/// FLaunchApplicationModule
///

Lumen::Launch::FLaunchApplicationModule::FLaunchApplicationModule ( TSharedPtr<FGenericApplication> InApplication ) noexcept
    : Application( std::move( InApplication ) ), MessageHandler( MakeShared<FLaunchMessageHandler>() )
{
    if ( Application.IsValid() )
    {
        Application->SetMessageHandler( MessageHandler );
        GApplication = Application;
    }
}

Lumen::Launch::FLaunchApplicationModule::~FLaunchApplicationModule () noexcept
{
    if ( GApplication == Application )
    {
        GApplication.Reset();
    }

    if ( Application.IsValid() )
    {
        Application->Shutdown();
        Application.Reset();
    }
}

void Lumen::Launch::FLaunchApplicationModule::Tick ( const Float64 InDeltaSeconds, const Float64 LUMEN_UNUSED InAlpha ) noexcept
{
    if ( Application.IsValid() )
    {
        Application->PumpMessages( InDeltaSeconds );
    }
}

Lumen::FStringView Lumen::Launch::FLaunchApplicationModule::GetName () const noexcept
{
    return "Application";
}

Lumen::Launch::EModuleTraits Lumen::Launch::FLaunchApplicationModule::GetTraits () const noexcept
{
    return EModuleTraits::Tick;
}

///
/// Accessors
///

Lumen::TSharedPtr<Lumen::FGenericApplication> Lumen::Launch::GetApplication () noexcept
{
    return GApplication;
}

Lumen::Launch::TLaunchResult<Lumen::TUniquePtr<Lumen::Launch::IEngineModule>>
Lumen::Launch::MakeApplicationModule ( const FLaunchEngineConfig &InConfig ) noexcept
{
    if ( InConfig.bHeadless )
    {
        return MakeUnique<FLaunchApplicationModule>( nullptr );
    }

#if defined( LUMEN_PLATFORM_LINUX )
    return MakeUnique<FLaunchApplicationModule>( FLinuxApplication::MakeLinuxApplication() );
#else
    return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                          "No platform application implementation for this target" );
#endif
}
