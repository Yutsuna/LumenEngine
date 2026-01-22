/**
 * @file LinuxBackend.hpp
 * @brief Declaration of the FLinuxBackend struct for Linux-specific backend management.
 */

#include "Linux/LinuxBackend.hpp"

#include <SDL3/SDL.h>

namespace
{

static LumenEngine::Bool GInitializedSDL = false;

} // namespace

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
