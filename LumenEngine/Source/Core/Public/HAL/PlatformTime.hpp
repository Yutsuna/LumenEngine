/**
 * @file PlatformTime.hpp
 * @brief Platform-specific time utilities for high-resolution timing and performance measurement.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

namespace LumenEngine
{

struct LUMEN_ENGINE_API FPlatformTime
{
    /** @returns The current time in seconds since the application started. */
    static Float64 Seconds ();

    /** Blocks the current thread for the specified number of seconds. */
    static void Sleep ( const Float64 Seconds );

    static constexpr Float64 DEFAULT_TICK_RATE = 1.0 / 60.0;
    static constexpr Float64 MAX_TICK_RATE     = 1.0 / 15.0;
};

} // namespace LumenEngine
