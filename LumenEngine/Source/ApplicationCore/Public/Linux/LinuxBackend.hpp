/**
 * @file LinuxBackend.hpp
 * @brief Declaration of the FLinuxBackend struct for Linux-specific backend management.
 */

#pragma once

#include "Definitions.hpp"
#include "CoreTypes.hpp"

namespace LumenEngine
{

struct LUMEN_ENGINE_API FLinuxBackend
{
    /** @return true if SDL library initialized successfully */
    static Bool InitializeSDL ();
    /** Shuts down the SDL library */
    static void ShutdownSDL ();
};

} // namespace LumenEngine
