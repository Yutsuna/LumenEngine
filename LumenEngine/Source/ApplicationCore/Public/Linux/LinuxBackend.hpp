/**
 * @file LinuxBackend.hpp
 * @brief Declaration of the FLinuxBackend struct for Linux-specific backend management.
 */

#pragma once

#include "Definitions.hpp"

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )

    #include "CoreTypes.hpp"

namespace LumenEngine
{

struct LUMEN_ENGINE_API FLinuxBackend
{
    /** @return true if SDL library initialized successfully */
    static Bool InitializeSDL ();
    /** Shuts down the SDL library */
    static void ShutdownSDL ();
    /** Pumps the SDL event queue, processing all pending events. */
    static void PumpMessages ();
};

} // namespace LumenEngine

#endif
