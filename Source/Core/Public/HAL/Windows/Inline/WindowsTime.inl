/**
 * @file WindowsTime.inl
 * @brief Inline implementation of Windows time functions.
 */

#pragma once

#include <HAL/Windows/WindowsTime.hpp>

/**
 * Private
 */

Lumen::HAL::Internal::FWindowsSleepTimer::FWindowsSleepTimer () noexcept
    : Handle( ::CreateWaitableTimerExW( nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS ) )
{
    /* */
}

Lumen::HAL::Internal::FWindowsSleepTimer::~FWindowsSleepTimer () noexcept
{
    if ( Handle != nullptr )
    {
        ::CloseHandle( Handle );
    }
}

Lumen::Float64 Lumen::HAL::Internal::QuerySecondsPerTick () noexcept
{
    static const Float64 Value = [] () noexcept
    {
        LARGE_INTEGER Frequency{};
        ::QueryPerformanceFrequency( &Frequency );
        return 1.0 / static_cast<Float64>( Frequency.QuadPart );
    }();
    return Value;
}

/**
 * Public
 */

Lumen::UInt64 Lumen::HAL::FWindowsTime::Ticks () noexcept
{
    LARGE_INTEGER Counter{};
    ::QueryPerformanceCounter( &Counter );
    return static_cast<UInt64>( Counter.QuadPart );
}

/** @returns The duration of @p DeltaTicks (a difference of two Ticks() values) in seconds. */
Lumen::Float64 Lumen::HAL::FWindowsTime::TicksToSeconds ( const UInt64 DeltaTicks ) noexcept
{
    return static_cast<Float64>( DeltaTicks ) * Internal::QuerySecondsPerTick();
}

/** @returns The current wall-clock time in nanoseconds since Unix epoch. */
Lumen::UInt64 Lumen::HAL::FWindowsTime::NowNanoseconds () noexcept
{
    // FILETIME counts 100 ns intervals since 1601-01-01. Offset to the Unix epoch (1970-01-01).
    constexpr UInt64 UnixEpochOffset = 116444736000000000ULL;

    FILETIME FileTime{};
    ::GetSystemTimePreciseAsFileTime( &FileTime );

    const UInt64 Intervals =
        ( static_cast<UInt64>( FileTime.dwHighDateTime ) << 32 ) | static_cast<UInt64>( FileTime.dwLowDateTime );
    return ( Intervals - UnixEpochOffset ) * 100ULL;
}
