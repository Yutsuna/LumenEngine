/**
 * @file WindowsMutex.inl
 * @brief Non-recursive mutex implementation
 */

#pragma once

#include <HAL/Windows/WindowsMutex.hpp>

/**
 * Public
 */

void Lumen::HAL::FWindowsMutex::Lock () noexcept
{
    AcquireSRWLockExclusive( &RWLock );
}

void Lumen::HAL::FWindowsMutex::Unlock () noexcept
{
    ReleaseSRWLockExclusive( &RWLock );
}

Lumen::Bool Lumen::HAL::FWindowsMutex::TryLock () noexcept
{
    return TryAcquireSRWLockExclusive( &RWLock ) != 0;
}
