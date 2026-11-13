/**
 * @file WindowsTime.cpp
 * @brief Windows time implementation.
 */

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_WINDOWS )

    #include <HAL/Windows/WindowsTime.hpp>

/**
 * Private
 */

namespace
{
/** 1 s = 1e7 unités de 100 ns (SetWaitableTimer) */
constexpr Lumen::Float64 TimerUnitsPerSecond = 1.0e7;

/** 1 s = 1e3 ms (::Sleep) */
constexpr Lumen::Float64 MillisecondsPerSecond = 1.0e3;

/** Max duration in seconds as DWORD */
constexpr Lumen::Float64 MaxSecondsForSleep = static_cast<Lumen::Float64>( DWORD_MAX ) / MillisecondsPerSecond;
} // namespace

/**
 * Public
 */

void Lumen::HAL::FWindowsTime::Sleep ( const Float64 Duration ) noexcept
{
    if ( not( Duration > 0.0 ) )
    {
        return;
    }

    const Float64 Clamped = Duration < MaxSleepSeconds ? Duration : MaxSleepSeconds;

    thread_local const Internal::FWindowsSleepTimer Timer;

    if ( Timer.Handle != nullptr )
    {
        LARGE_INTEGER DueTime;
        DueTime.QuadPart = -( static_cast<LONGLONG>( ( Clamped * TimerUnitsPerSecond ) ) + 1 );

        if ( ::SetWaitableTimer( Timer.Handle, &DueTime, 0, nullptr, nullptr, FALSE ) != FALSE )
        {
            ::WaitForSingleObject( Timer.Handle, INFINITE );
            return;
        }
    }

    const Float64 Safe         = Clamped < MaxSecondsForSleep ? Clamped : MaxSecondsForSleep;
    const Float64 Milliseconds = Safe * MillisecondsPerSecond;

    DWORD Whole = static_cast<DWORD>( Milliseconds );
    if ( static_cast<Float64>( Whole ) < Milliseconds )
    {
        ++Whole;
    }

    ::Sleep( Whole );
}

#endif
