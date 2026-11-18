/**
 * @file WindowsSpinLockMutex.inl
 * @brief Busy-wait spin lock implementation.
 */

#pragma once

#include <HAL/PlatformAtomic.hpp>
#include <HAL/Windows/WindowsSpinLockMutex.hpp>

/**
 * Public
 */

void Lumen::HAL::FWindowsSpinLockMutex::Lock () noexcept
{
    UInt32 SpinCount = 0U;

    while ( Flag.test_and_set( std::memory_order::acquire ) )
    {
        while ( Flag.test( std::memory_order::relaxed ) )
        {
            if ( SpinCount < GMaxRelaxSpins )
            {
                CpuRelax();
                ++SpinCount;
            }
            else
            {
                static_cast<void>( SwitchToThread() );
            }
        }
    }
}

void Lumen::HAL::FWindowsSpinLockMutex::Unlock () noexcept
{
    Flag.clear( std::memory_order::release );
}

Lumen::Bool Lumen::HAL::FWindowsSpinLockMutex::IsLocked () const noexcept
{
    return Flag.test( std::memory_order::relaxed );
}

Lumen::Bool Lumen::HAL::FWindowsSpinLockMutex::TryLock () noexcept
{
    return not Flag.test_and_set( std::memory_order::acquire );
}
