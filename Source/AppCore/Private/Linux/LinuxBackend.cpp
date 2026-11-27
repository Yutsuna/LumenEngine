/**
 * @file LinuxBackend.cpp
 * @brief Implementation of the FLinuxBackend struct for Linux-specific backend management
 */

#include <Linux/LinuxBackend.hpp>

#if defined( LUMEN_PLATFORM_LINUX )

    #include <SDL3/SDL.h>

    #include <Linux/LinuxApplication.hpp>
    #include <Logging/Logger.hpp>

namespace
{

Lumen::TAtomic<Lumen::Bool> GInitializedSDL{ false };
const Lumen::FLogCategory LogLinuxBackend( "LinuxBackend" );

inline void FlushEventQueue () noexcept
{
    SDL_Event Event;
    while ( SDL_PollEvent( &Event ) )
    {
        /** Discard */
    }
}

} // namespace

Lumen::Bool Lumen::FLinuxBackend::InitializeSDL () noexcept
{
    if ( GInitializedSDL.load( std::memory_order_relaxed ) ) [[likely]]
    {
        return true;
    }
    if ( not SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) ) [[unlikely]]
    {
        return false;
    }

    SDL_EnableScreenSaver();
    GInitializedSDL.store( true, std::memory_order_release );
    return true;
}

void Lumen::FLinuxBackend::ShutdownSDL () noexcept
{
    if ( GInitializedSDL.load( std::memory_order_relaxed ) ) [[likely]]
    {
        SDL_Quit();
        GInitializedSDL.store( false, std::memory_order_release );
    }
}

void Lumen::FLinuxBackend::PumpSDLMessagesQueue () noexcept
{
    if ( not GInitializedSDL.load( std::memory_order_relaxed ) ) [[unlikely]]
    {
        LUMEN_LOG_WARNING( LogLinuxBackend, "Attempted to pump messages without SDL being initialized." );
        return;
    }

    Lumen::FLinuxApplication *const Application = GLinuxApplication.load( std::memory_order_acquire );
    if ( Application == nullptr ) [[unlikely]]
    {
        FlushEventQueue();
        LUMEN_LOG_WARNING( LogLinuxBackend, "Attempted to pump messages without a valid Linux Application instance." );
        return;
    }

    SDL_Event Event;
    while ( SDL_PollEvent( &Event ) )
    {
        Application->AddPendingEvent( Event );
    }
}

#endif
