/**
 * @file UnixTime.cpp
 * @brief Implements the FUnixTime class for Unix-based platforms.
 */

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_UNIX )

    #include <HAL/Unix/UnixTime.hpp>

extern "C"
{
    #include <errno.h>
}

/**
 * Public
 */

void Lumen::HAL::FUnixTime::Sleep ( const Float64 Duration ) noexcept
{
    if ( not( Duration > 0.0 ) )
    {
        return;
    }

    const Float64 Clamped       = Duration < MaxSleepSeconds ? Duration : MaxSleepSeconds;
    const time_t Whole          = static_cast<time_t>( Clamped );
    const Float64 FloatNanonSec = static_cast<Float64>( NanosecondsPerSecond );
    Int64 Nanoseconds           = static_cast<Int64>( ( Clamped - static_cast<Float64>( Whole ) ) * FloatNanonSec );

    if ( Nanoseconds > MaxNanoseconds )
    {
        Nanoseconds = MaxNanoseconds;
    }

    timespec Request = {};
    Request.tv_sec   = Whole;
    Request.tv_nsec  = Nanoseconds;

    timespec Remaining = {};
    while ( ::nanosleep( &Request, &Remaining ) == -1 && errno == EINTR )
    {
        Request = Remaining;
    }
}

#endif
