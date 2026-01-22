/**
 * @file LinuxBackend.hpp
 * @brief Declaration of the FLinuxBackend struct for Linux-specific backend management.
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

struct FLinuxBackend
{
    /** @return true if SDL library initialized successfully */
    static Bool InitializeSDL ();
    /** Shuts down the SDL library */
    static void ShutdownSDL ();
};

} // namespace LumenEngine
