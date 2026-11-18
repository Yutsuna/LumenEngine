/**
 * @file PlatformTime.cpp
 * @brief Platform-specific time utilities for high-resolution timing and performance measurement.
 */

#include <HAL/PlatformTime.hpp>

Lumen::Float64 Lumen::HAL::FPlatformTime::Seconds () noexcept
{
    /**
     * INFO: thread-safe, initialised on first call. The second Ticks() is read after the baseline is
     *       visible to this thread, so Now >= StartTicks always holds (no unsigned underflow).
     */
    static const UInt64 StartTicks = Ticks();
    const UInt64 Now               = Ticks();

    return TicksToSeconds( Now - StartTicks );
}
