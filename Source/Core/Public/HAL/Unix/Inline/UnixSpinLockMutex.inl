/**
 * @file UnixSpinLockMutex.inl
 * @brief Inline implementation of the Unix spin lock mutex.
 */

#pragma once

#include <HAL/PlatformAtomic.hpp>

#include <HAL/Unix/UnixSpinLockMutex.hpp>

#include <sched.h>

/**
 * Public
 */

void Lumen::HAL::FUnixSpinLockMutex::Lock () noexcept
{
    UInt32 SpinCount = 0;

    /** INFO: On test_and_set failure, spin on a relaxed load until the flag looks free */
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
                static_cast<void>( sched_yield() );
            }
        }
    }
}

void Lumen::HAL::FUnixSpinLockMutex::Unlock () noexcept
{
    Flag.clear( std::memory_order::release );
}

Lumen::Bool Lumen::HAL::FUnixSpinLockMutex::TryLock () noexcept
{
    return not Flag.test_and_set( std::memory_order::acquire );
}

Lumen::Bool Lumen::HAL::FUnixSpinLockMutex::IsLocked () const noexcept
{
    return Flag.test( std::memory_order::relaxed );
}
