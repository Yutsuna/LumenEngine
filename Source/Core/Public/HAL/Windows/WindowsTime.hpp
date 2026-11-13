/**
 * @file WindowsTime.hpp
 * @brief Time utilities for Windows built directly on the Win32 API.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

// TODO inverse
#if defined( LUMEN_PLATFORM_WINDOWS )
    #error "WindowsTime.hpp is only supported on Windows"
#else

extern "C"
{
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>
}

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <CoreExport.hpp>

    #include <HAL/Generic/GenericTime.hpp>

    #ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
        #define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
    #endif

namespace Lumen
{

namespace HAL
{

    namespace Internal
    {
        /** @brief Per-thread high-resolution waitable timer. Handle is null if unsupported (Windows < 10 1803). */
        struct FWindowsSleepTimer : public FNonCopyable
        {
            HANDLE Handle;

            LUMEN_FORCE_INLINE FWindowsSleepTimer () noexcept;
            LUMEN_FORCE_INLINE ~FWindowsSleepTimer() noexcept;

            FWindowsSleepTimer ( const FWindowsSleepTimer & )           = delete;
            FWindowsSleepTimer &operator=( const FWindowsSleepTimer & ) = delete;
        };

        /** @returns 1 / QueryPerformanceFrequency. The frequency is fixed at boot, so it is queried once. */
        LUMEN_FORCE_INLINE Float64 QuerySecondsPerTick () noexcept;

    } // namespace Internal

    /**
     * @struct FWindowsTime
     * @brief Time utilities for Windows.
     */
    struct CORE_EXPORT FWindowsTime : FGenericPlatformTime
    {
        /** @returns The current time in seconds since the application started (first call). */
        CORE_EXPORT static Float64 Seconds () noexcept;

        /**
         * @returns A raw monotonic QueryPerformanceCounter value. The origin is arbitrary,
         *          only differences between two values are meaningful.
         */
        static LUMEN_FORCE_INLINE UInt64 Ticks () noexcept;

        /** @returns The duration of @p DeltaTicks (a difference of two Ticks() values) in seconds. */
        static LUMEN_FORCE_INLINE Float64 TicksToSeconds ( const UInt64 DeltaTicks ) noexcept;

        /** @returns The current wall-clock time in nanoseconds since Unix epoch. */
        static LUMEN_FORCE_INLINE UInt64 NowNanoseconds () noexcept;

        /**
         * @brief Blocks the current thread for at least the specified number of seconds.
         * @note  Uses a high-resolution waitable timer (sub-millisecond accuracy) when available,
         *        otherwise falls back to ::Sleep (rounded up to whole milliseconds).
         *        Non-positive and NaN durations return immediately.
         */
        static void Sleep ( const Float64 Duration ) noexcept;
    };

} // namespace HAL

} // namespace Lumen

#endif
