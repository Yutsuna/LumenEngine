/**
 * @file PlatformTime.hpp
 * @brief Platform-specific time utilities for high-resolution timing and performance measurement.
 */

#pragma once

#if defined( _GLIBCXX_CHRONO_H )
    #warning "<chrono> include detected. Please prefer only using <HAL/PlatformTime.hpp>."
#endif

#include <Core/CoreDefinitions.hpp>

#include <HAL/Generic/GenericTime.hpp>

#if defined( LUMEN_PLATFORM_WINDOWS )
    #include <HAL/Windows/WindowsTime.hpp>
#elif defined( LUMEN_PLATFORM_UNIX )
    #include <HAL/Unix/UnixTime.hpp>
#else
    #error "PlatformTime.hpp is unable to determine the platform."
#endif

namespace Lumen
{

namespace HAL
{

#if defined( LUMEN_PLATFORM_WINDOWS )
    using FPlatformTime = FWindowsTime;
#else
    using FPlatformTime = FUnixTime;
#endif

} // namespace HAL

#if defined( LUMEN_COMPILER_MSVC )
    #pragma warning( push )
    #pragma warning( disable : 4455 )

#elif defined( LUMEN_COMPILER_CLANG )
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wuser-defined-literals"

#elif defined( LUMEN_COMPILER_GCC )
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wliteral-suffix"

#endif

[[nodiscard]] constexpr Float64 operator""ns( UInt64 Value ) noexcept
{
    return HAL::FGenericPlatformTime::ToSeconds<1.0 / 1'000'000'000.0>( Value );
}

[[nodiscard]] constexpr Float64 operator""us( UInt64 Value ) noexcept
{
    return HAL::FGenericPlatformTime::ToSeconds<1.0 / 1'000'000.0>( Value );
}

[[nodiscard]] constexpr Float64 operator""ms( UInt64 Value ) noexcept
{
    return HAL::FGenericPlatformTime::ToSeconds<1.0 / 1'000.0>( Value );
}

[[nodiscard]] constexpr Float64 operator""s( UInt64 Value ) noexcept
{
    return HAL::FGenericPlatformTime::ToSeconds<1.0>( Value );
}

[[nodiscard]] constexpr Float64 operator""min( UInt64 Value ) noexcept
{
    return HAL::FGenericPlatformTime::ToSeconds<60.0>( Value );
}

[[nodiscard]] constexpr Float64 operator""h( UInt64 Value ) noexcept
{
    return HAL::FGenericPlatformTime::ToSeconds<3'600.0>( Value );
}

#if defined( LUMEN_COMPILER_MSVC )
    #pragma warning( pop )

#elif defined( LUMEN_COMPILER_CLANG )
    #pragma clang diagnostic pop

#elif defined( LUMEN_COMPILER_GCC )
    #pragma GCC diagnostic pop

#endif

} // namespace Lumen
