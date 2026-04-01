/**
 * @file LinuxBackend.hpp
 * @brief Declaration of the FLinuxBackend struct for Linux-specific backend management.
 */

#include "Linux/LinuxBackend.hpp"

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )

    #include "Linux/LinuxApplication.hpp"

    #include "Logging/Logger.hpp"
    #include "Logging/LoggingCategory.hpp"

    #include <SDL3/SDL.h>

const LumenEngine::FLogCategory LogLinuxBackend( "LinuxBackend" );
LumenEngine::Bool GInitializedSDL = false;

LumenEngine::Bool LumenEngine::FLinuxBackend::InitializeSDL ()
{
    if ( GInitializedSDL )
    {
        return true;
    }

    if ( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) )
    {
        return false;
    }

    SDL_EnableScreenSaver();
    GInitializedSDL = true;
    return true;
}

void LumenEngine::FLinuxBackend::ShutdownSDL ()
{
    if ( GInitializedSDL )
    {
        SDL_Quit();
        GInitializedSDL = false;
    }
}

namespace
{

static inline void FlushEventQueue () noexcept
{
    SDL_Event Event;

    while ( SDL_PollEvent( &Event ) )
    {
        /* Discard all events */
    }
}

} // namespace

void LumenEngine::FLinuxBackend::PumpMessages ()
{
    if ( not GInitializedSDL )
    {
        LUMEN_LOG_WARNING( LogLinuxBackend, "Attempted to pump messages without SDL being initialized." );
        return;
    }
    if ( not GLinuxApplication )
    {
        FlushEventQueue();
        LUMEN_LOG_WARNING( LogLinuxBackend, "Attempted to pump messages without a valid Linux application instance." );
        return;
    }

    SDL_Event Event;

    while ( SDL_PollEvent( &Event ) )
    {
        GLinuxApplication->AddPendingEvent( Event );
    }
}

#endif
