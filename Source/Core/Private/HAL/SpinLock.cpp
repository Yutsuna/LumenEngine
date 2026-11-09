/**
 * @file SpinLock.cpp
 * @brief Implementation of the FSpinLock class.
 */

#include <Core/CoreDefinitions.hpp>

#include <HAL/PlatformAtomic.hpp>
#include <HAL/SpinLock.hpp>

/**
 * FSpinLock
 */

void Lumen::HAL::FSpinLock::Lock () noexcept
{

    /**
     * INFO: TTAS on CAS failure, spin on a relaxed load until the flag looks free.
     * Then retry the cache-line-invalidating test_and_set.
     */
    while ( MutexFlag.test_and_set( std::memory_order::acquire ) )
    {
        while ( MutexFlag.test( std::memory_order::relaxed ) )
        {
            CpuRelax();
        }
    }
}

Lumen::Bool Lumen::HAL::FSpinLock::TryLock () noexcept
{
    return not MutexFlag.test_and_set( std::memory_order::acquire );
}

void Lumen::HAL::FSpinLock::Unlock () noexcept
{
    MutexFlag.clear( std::memory_order::release );
}

Lumen::Bool Lumen::HAL::FSpinLock::IsLocked () const noexcept
{
    return MutexFlag.test( std::memory_order::relaxed );
}

Lumen::HAL::FSpinLock::operator Bool () const noexcept
{
    return IsLocked();
}

/**
 * FSpinGuard
 */

Lumen::HAL::FSpinGuard::FSpinGuard ( FSpinLock &InLock ) noexcept : Lock( InLock )
{
    Lock.Lock();
}

Lumen::HAL::FSpinGuard::~FSpinGuard () noexcept
{
    Lock.Unlock();
}
