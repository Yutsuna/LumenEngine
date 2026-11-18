/**
 * @file FMacOSFutex.cpp
 * @brief Implementation of FMacOSFutex.
 */

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_MACOS )

    #include <HAL/MacOS/MacOSFutex.hpp>

extern "C"
{

    #include <errno.h>
    #include <os/os_sync_wait_on_address.h>
}

/**
 * Public
 */

void Lumen::HAL::FMacOSFutex::Wait ( TAtomic<UInt32> &InAddress, UInt32 InExpected ) noexcept
{
    static_cast<void>( os_sync_wait_on_address( Raw( InAddress ), InExpected, sizeof( UInt32 ), OS_SYNC_WAIT_ON_ADDRESS_NONE ) );
}

Lumen::Bool Lumen::HAL::FMacOSFutex::WaitFor ( TAtomic<UInt32> &InAddress, UInt32 InExpected, Float64 InTimeoutSeconds ) noexcept
{
    const UInt64 Nanoseconds = static_cast<UInt64>( ClampTimeout( InTimeoutSeconds ) * 1.0e9 );

    const Int32 Result = os_sync_wait_on_address_with_timeout(
        Raw( InAddress ), InExpected, sizeof( UInt32 ), OS_SYNC_WAIT_ON_ADDRESS_NONE, OS_CLOCK_MACH_ABSOLUTE_TIME, Nanoseconds );

    return not( Result == -1 and errno == ETIMEDOUT );
}

void Lumen::HAL::FMacOSFutex::WakeOne ( TAtomic<UInt32> &InAddress ) noexcept
{
    static_cast<void>( os_sync_wake_by_address_any( Raw( InAddress ), sizeof( UInt32 ), OS_SYNC_WAKE_BY_ADDRESS_NONE ) );
}

void Lumen::HAL::FMacOSFutex::WakeAll ( TAtomic<UInt32> &InAddress ) noexcept
{
    static_cast<void>( os_sync_wake_by_address_all( Raw( InAddress ), sizeof( UInt32 ), OS_SYNC_WAKE_BY_ADDRESS_NONE ) );
}

/**
 * Private
 */

Lumen::UInt32 *Lumen::HAL::FMacOSFutex::Raw ( TAtomic<UInt32> &InAddress ) noexcept
{
    /** INFO: TAtomic<UInt32> is lock-free and 4 bytes (static_asserts in the header), so this is the usual futex idiom */
    return reinterpret_cast<UInt32 *>( &InAddress );
}

Lumen::Float64 Lumen::HAL::FMacOSFutex::ClampTimeout ( Float64 InSeconds ) noexcept
{
    constexpr Float64 MaxSeconds = 31536000.0; /** 1 year */

    /** INFO: Written so a NaN falls in the first branch */
    if ( not( InSeconds > 0.0 ) )
    {
        return 0.0;
    }

    return InSeconds < MaxSeconds ? InSeconds : MaxSeconds;
}

#endif
