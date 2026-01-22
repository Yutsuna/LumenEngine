/**
 * @file LinuxApplication.cpp
 * @brief Implementation of the FLinuxApplication class for Linux-specific application management.
 */

#include "Linux/LinuxApplication.hpp"
#include "Linux/LinuxBackend.hpp"
#include "Linux/LinuxWindow.hpp"

#include "Generic/GenericApplicationMessageHandler.hpp"

#include <SDL3/SDL.h>

/**
 * public
 */

LumenEngine::FLinuxApplication::FLinuxApplication ()
    : FGenericApplication()
{
    if ( not FLinuxBackend::InitializeSDL() )
    {
        /* TODO: log error */
        return;
    }
}

LumenEngine::FLinuxApplication::~FLinuxApplication ()
{
    FLinuxBackend::ShutdownSDL();
}

LumenEngine::TSharedRef<LumenEngine::FGenericWindow> LumenEngine::FLinuxApplication::MakeWindow ()
{
    return FLinuxWindow::Make();
}

LumenEngine::TSharedRef<LumenEngine::FGenericApplication> LumenEngine::FLinuxApplication::CreateLinuxApplication ()
{
    return MakeShared<FLinuxApplication>();
}

void LumenEngine::FLinuxApplication::InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow, const TSharedRef<FGenericWindowDescription> &InDescription, const TSharedPtr<FGenericWindow> &InParentWindow, const bool bShowImmediately )
{
    TSharedRef<FLinuxWindow> LinuxWindow = MakeShareable<FLinuxWindow>( static_cast<FLinuxWindow *>( InWindow.Get() ) );

    LinuxWindow->Initialize( this, InDescription, InParentWindow, bShowImmediately );
    Windows.push_back( LinuxWindow );
}

void LumenEngine::FLinuxApplication::PumpMessages ( const Float32 DeltaTime )
{
    SDL_Event Event;

    while ( SDL_PollEvent( &Event ) )
    {
        switch ( Event.type )
        {
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            break;

        case SDL_EVENT_MOUSE_MOTION:
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            break;

        case SDL_EVENT_QUIT:
            break;

        default:
            break;
        }
    }
}
