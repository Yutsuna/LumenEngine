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

#elif defined( LUMEN_PLATFORM_UNIX )
    #include <HAL/Unix/UnixMutex.hpp>
    #include <HAL/Unix/UnixRecursiveMutex.hpp>
    #include <HAL/Unix/UnixSharedMutex.hpp>
    #include <HAL/Unix/UnixSpinLockMutex.hpp>

#else
    #error "PlatformMutex.hpp is unable to determine the platform."
#endif
