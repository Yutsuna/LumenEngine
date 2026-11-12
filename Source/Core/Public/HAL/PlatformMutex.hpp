/**
 * @file PlatformMutex.hpp
 * @brief Platform mutex
 */

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_WINDOWS )
    #include <HAL/Windows/WindowsMutex.hpp>
    #include <HAL/Windows/WindowsRecursiveMutex.hpp>
    #include <HAL/Windows/WindowsSharedMutex.hpp>
    #include <HAL/Windows/WindowsSpinLockMutex.hpp>

namespace Lumen
{

namespace HAL
{

    using FPlatformMutex          = FWindowsMutex;
    using FPlatformRecursiveMutex = FWindowsRecursiveMutex;
    using FPlatformSharedMutex    = FWindowsSharedMutex;
    using FPlatformSpinLockMutex  = FWindowsSpinLockMutex;

} // namespace HAL

} // namespace Lumen

#elif defined( LUMEN_PLATFORM_UNIX )
    #include <HAL/Unix/UnixMutex.hpp>
    #include <HAL/Unix/UnixRecursiveMutex.hpp>
    #include <HAL/Unix/UnixSharedMutex.hpp>
    #include <HAL/Unix/UnixSpinLockMutex.hpp>

namespace Lumen
{

namespace HAL
{

    using FPlatformMutex          = FUnixMutex;
    using FPlatformRecursiveMutex = FUnixRecursiveMutex;
    using FPlatformSharedMutex    = FUnixSharedMutex;
    using FPlatformSpinLockMutex  = FUnixSpinLockMutex;

} // namespace HAL

} // namespace Lumen

#else
    #error "PlatformMutex.hpp is unable to determine the platform."
#endif
