///
/// @file PlatformTime.hpp
/// @brief Platform-specific time utilities for high-resolution timing and performance measurement.
///

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

namespace Lumen
{

namespace HAL
{

    ///
    /// @struct FPlatformTime
    /// @brief Platform-specific time utilities for high-resolution timing and performance measurement.
    ///
    struct CORE_EXPORT FPlatformTime
    {
        /// @returns The current time in seconds since the application started.
        static Float64 Seconds () noexcept;

        /// @returns The current wall-clock time in nanoseconds since Unix epoch.
        static UInt64 NowNanoseconds () noexcept;

        /// @brief Blocks the current thread for the specified number of seconds.
        static void Sleep ( const Float64 Seconds ) noexcept;

        static constexpr Float64 DefaultTickRate = 1.0 / 60.0;
        static constexpr Float64 MaxTickRate     = 1.0 / 15.0;
    };
} // namespace HAL

} // namespace Lumen