/**
 * @file LinuxApplication.cpp
 * @brief Implementation of the FLinuxApplication class for Linux-specific application management.
 */

#include "Linux/LinuxApplication.hpp"
#include "Linux/LinuxBackend.hpp"
#include "Linux/LinuxWindow.hpp"

#include "Generic/GenericApplicationMessageHandler.hpp"

#include <SDL3/SDL.h>

LumenEngine::FLinuxApplication::FLinuxApplication () : FGenericApplication()
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

void LumenEngine::FLinuxApplication::InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                                                        const TSharedRef<FGenericWindowDescription> &InDescription,
                                                        const TSharedPtr<FGenericWindow> &InParentWindow,
                                                        const bool bShowImmediately )
{
    const TSharedRef<FLinuxWindow> LinuxWindow  = StaticCastSharedRef<FLinuxWindow>( InWindow );
    const TSharedPtr<FLinuxWindow> ParentWindow = StaticCastSharedPtr<FLinuxWindow>( InParentWindow );

    LinuxWindow->Initialize( this, InDescription, InParentWindow, bShowImmediately );
    Windows.push_back( LinuxWindow );
}

namespace
{

static inline void FlushEventQueue ( SDL_Event *Event ) noexcept
{
    while ( SDL_PollEvent( Event ) )
    {
        /* Discard all events */
    }
}

} // namespace

void LumenEngine::FLinuxApplication::PumpMessages ( const Float32 DeltaTime )
{
    SDL_Event Event;

    if ( not MessageHandler.IsValid() )
    {
        FlushEventQueue( &Event );
        return;
    }

    while ( SDL_PollEvent( &Event ) )
    {
        switch ( Event.type )
        {
        default:
            break;
        }
    }
}
