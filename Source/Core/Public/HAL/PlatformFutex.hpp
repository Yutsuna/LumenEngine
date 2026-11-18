/**
 * @file PlatformFutex.hpp
 * @brief Address-based wait / wake primitive.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace HAL
{

    static_assert( sizeof( TAtomic<UInt32> ) == sizeof( UInt32 ), "TAtomic<UInt32> must be exactly 4 bytes." );
    static_assert( TAtomic<UInt32>::is_always_lock_free, "TAtomic<UInt32> must be lock-free to be used as a futex word." );

} // namespace HAL

} // namespace Lumen

#if defined( LUMEN_PLATFORM_LINUX )
    #include <HAL/Linux/LinuxFutex.hpp>

namespace Lumen
{

namespace HAL
{
    using FPlatformFutex = FLinuxFutex;
} // namespace HAL

} // namespace Lumen

#elif defined( LUMEN_PLATFORM_MACOS )
    #include <HAL/MacOS/MacOSFutex.hpp>

namespace Lumen
{

namespace HAL
{
    using FPlatformFutex = FMacOSFutex;
} // namespace HAL

} // namespace Lumen

#elif defined( LUMEN_PLATFORM_WINDOWS )
    #include <HAL/Windows/WindowsFutex.hpp>

namespace Lumen
{

namespace HAL
{
    using FPlatformFutex = FWindowsFutex;
} // namespace HAL

} // namespace Lumen

#endif
