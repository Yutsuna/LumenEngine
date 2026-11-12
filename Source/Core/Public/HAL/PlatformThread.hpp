/**
 * @file PlatformThread.hpp
 * @brief Selects the native thread implementation.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_UNIX )
    #include <HAL/Unix/UnixThread.hpp>

namespace Lumen
{
namespace HAL
{
    using FPlatformThread = FUnixThread;
} // namespace HAL
} // namespace Lumen

#elif defined( LUMEN_PLATFORM_WINDOWS )
    #include <HAL/Windows/WindowsThread.hpp>

namespace Lumen
{
namespace HAL
{
    using FPlatformThread = FWindowsThread;
} // namespace HAL
} // namespace Lumen

#else
    #error "PlatformThread.hpp is unable to determine the platform."
#endif
