/**
 * @file UnixTime.inl
 * @brief Inline implementation of Unix time functions.
 */

#pragma once

#include <HAL/Unix/UnixTime.hpp>

extern "C"
{
#include <time.h>
}

/**
 * Public
 */

Lumen::UInt64 Lumen::HAL::FUnixTime::Ticks () noexcept
{
#if defined( LUMEN_PLATFORM_APPLE )
    return static_cast<UInt64>( ::clock_gettime_nsec_np( CLOCK_UPTIME_RAW ) );
#elif defined( LUMEN_PLATFORM_LINUX )
    timespec Time{};
    ::clock_gettime( CLOCK_MONOTONIC, &Time );
    return ( static_cast<UInt64>( Time.tv_sec ) * NanosecondsPerSecond ) + static_cast<UInt64>( Time.tv_nsec );
#endif
}

constexpr Lumen::Float64 Lumen::HAL::FUnixTime::TicksToSeconds ( const UInt64 DeltaTicks ) noexcept
{
    return static_cast<Float64>( DeltaTicks ) * SecondsPerNanosecond;
}

Lumen::UInt64 Lumen::HAL::FUnixTime::NowNanoseconds () noexcept
{
    timespec Time{};

    ::clock_gettime( CLOCK_REALTIME, &Time );
    return ( static_cast<UInt64>( Time.tv_sec ) * NanosecondsPerSecond ) + static_cast<UInt64>( Time.tv_nsec );
}
