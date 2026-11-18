/**
 * @file LinuxFutex.cpp
 * @brief Implementation of FLinuxFutex.
 */

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_LINUX )

    #include <HAL/Linux/LinuxFutex.hpp>

extern "C"
{
    #include <errno.h>
    #include <limits.h>
    #include <linux/futex.h>
    #include <sys/syscall.h>
    #include <unistd.h>
}

    #include <cmath>

/**
 * Public
 */

void Lumen::HAL::FLinuxFutex::Wait ( TAtomic<UInt32> &InAddress, UInt32 InExpected ) noexcept
{
    /** INFO: EAGAIN (value differs) and EINTR are both fine: callers re-check their condition */
    static_cast<void>( ::syscall( SYS_futex, Raw( InAddress ), FUTEX_WAIT_PRIVATE, InExpected, nullptr, nullptr, 0 ) );
}

Lumen::Bool Lumen::HAL::FLinuxFutex::WaitFor ( TAtomic<UInt32> &InAddress, UInt32 InExpected, Float64 InTimeoutSeconds ) noexcept
{
    const Float64 Clamped = ClampTimeout( InTimeoutSeconds );
    const Float64 Whole   = std::floor( Clamped );

    /** INFO: FUTEX_WAIT takes a RELATIVE timeout (CLOCK_MONOTONIC) */
    timespec Timeout{};
    Timeout.tv_sec  = static_cast<time_t>( Whole );
    Timeout.tv_nsec = static_cast<Int64>( ( Clamped - Whole ) * 1.0e9 );

    const Int64 Result = ::syscall( SYS_futex, Raw( InAddress ), FUTEX_WAIT_PRIVATE, InExpected, &Timeout, nullptr, 0 );

    return not( Result == -1 and errno == ETIMEDOUT );
}

void Lumen::HAL::FLinuxFutex::WakeOne ( TAtomic<UInt32> &InAddress ) noexcept
{
    static_cast<void>( ::syscall( SYS_futex, Raw( InAddress ), FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0 ) );
}

void Lumen::HAL::FLinuxFutex::WakeAll ( TAtomic<UInt32> &InAddress ) noexcept
{
    static_cast<void>( ::syscall( SYS_futex, Raw( InAddress ), FUTEX_WAKE_PRIVATE, INT_MAX, nullptr, nullptr, 0 ) );
}

/**
 * Private
 */

Lumen::UInt32 *Lumen::HAL::FLinuxFutex::Raw ( TAtomic<UInt32> &InAddress ) noexcept
{
    /** INFO: TAtomic<UInt32> is lock-free and 4 bytes */
    return reinterpret_cast<UInt32 *>( &InAddress );
}

Lumen::Float64 Lumen::HAL::FLinuxFutex::ClampTimeout ( Float64 InSeconds ) noexcept
{
    static constexpr Float64 MaxSeconds = 31536000.0; /** 1 year */

    /** INFO: Written so a NaN falls in the first branch */
    if ( not( InSeconds > 0.0 ) )
    {
        return 0.0;
    }

    return InSeconds < MaxSeconds ? InSeconds : MaxSeconds;
}

#endif
