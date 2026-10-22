/// @file LinuxBackend.cpp
/// @brief Implementation of the FLinuxBackend struct for Linux-specific backend management

#include <Linux/LinuxBackend.hpp>

#if defined( LUMEN_PLATFORM_LINUX )

#include <SDL3/SDL.h>

#include <Logging/Logger.hpp>

namespace
{

    Lumen::Bool GInitializedSDL = false;
    const Lumen::FLogCategory LogLinuxBackend("LinuxBackend");


    void FlushEventQueue() noexcept
    {
        SDL_Event Event;
        while ( SDL_PollEvent(&Event))
        {
            /// Discard
        }
    }

}

Lumen::Bool Lumen::FLinuxBackend::InitializeSDL () noexcept
{
    if ( GInitializedSDL )
    {
        return true;
    }
    if ( not SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ))
    {
        return false;
    }

    SDL_EnableScreenSaver();
    GInitializedSDL = true;
    return true;
}

void Lumen::FLinuxBackend::ShutdownSDL() noexcept
{

    if ( GInitializedSDL )
    {
        SDL_Quit();
        GInitializedSDL = false;
    }

}

void Lumen::FLinuxBackend::PumpSDLMessagesQueue () noexcept
{
    if ( not GInitializedSDL )
    {
        LUMEN_LOG_WARNING( LogLinuxBackend, "Attempted to pump messages without SDL being initialized." );
        return;
    }

    if ( not GLinuxApplication )
    {
        FlushEventQueue();
        LUMEN_LOG_WARNING(LogLinuxBackend, "Attempted to pump messages without a valid Linux Application instance.");
        return;
    }

    SDL_Event Event;
    while ( SDL_PollEvent( &Event ))
    {
        GLinuxApplication->AddPendingEvent( Event );
    }
}

#endif