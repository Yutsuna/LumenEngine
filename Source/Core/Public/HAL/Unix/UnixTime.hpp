/**
 * @file UnixTime.hpp
 * @brief Time utilities for Unix-like platforms (Linux, macOS) built directly on POSIX.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_UNIX )
    #error "UnixTime.hpp is only supported on Unix-like platforms"
#else

    #include <Core/CoreTypes.hpp>
    #include <CoreExport.hpp>

    #include <HAL/Generic/GenericTime.hpp>

namespace Lumen
{

namespace HAL
{

    /**
     * @struct FUnixTime
     * @brief Time utilities for Linux and macOS.
     */
    struct CORE_EXPORT FUnixTime : FGenericPlatformTime
    {
        /** @returns The current time in seconds since the application started (first call). */
        static Float64 Seconds () noexcept;

        /**
         * @returns A raw monotonic tick count in nanoseconds. The origin is arbitrary,
         *          only differences between two values are meaningful.
         */
        static LUMEN_FORCE_INLINE UInt64 Ticks () noexcept;

        /** @returns The duration of @p DeltaTicks (a difference of two Ticks() values) in seconds. */
        static constexpr Float64 TicksToSeconds ( const UInt64 DeltaTicks ) noexcept;

        /** @returns The current wall-clock time in nanoseconds since Unix epoch. */
        static LUMEN_FORCE_INLINE UInt64 NowNanoseconds () noexcept;

        /**
         * @brief Blocks the current thread for at least the specified number of seconds.
         * @note  Non-positive and NaN durations return immediately. Interrupted sleeps are resumed.
         */
        static void Sleep ( const Float64 Duration ) noexcept;

    private:

        static constexpr UInt64 NanosecondsPerSecond  = 1'000'000'000ULL;
        static constexpr Float64 SecondsPerNanosecond = 1.0 / static_cast<Float64>( NanosecondsPerSecond );
        static constexpr Int64 MaxNanoseconds         = static_cast<Int64>( NanosecondsPerSecond ) - 1;
    };

} // namespace HAL

} // namespace Lumen

    #include "Inline/UnixTime.inl"

#endif
