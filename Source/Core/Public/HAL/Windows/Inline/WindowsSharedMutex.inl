/**
 * @file WindowsSharedMutex.inl
 * @brief Readers-writer mutex implementation.
 */

#pragma once

#include <HAL/Windows/WindowsSharedMutex.hpp>

/**
 * Public
 */

void Lumen::HAL::FWindowsSharedMutex::Lock () noexcept
{
    AcquireSRWLockExclusive( &Mutex );
}

void Lumen::HAL::FWindowsSharedMutex::Unlock () noexcept
{
    ReleaseSRWLockExclusive( &Mutex );
}

void Lumen::HAL::FWindowsSharedMutex::LockShared () noexcept
{
    AcquireSRWLockShared( &Mutex );
}

void Lumen::HAL::FWindowsSharedMutex::UnlockShared () noexcept
{
    ReleaseSRWLockShared( &Mutex );
}

Lumen::Bool Lumen::HAL::FWindowsSharedMutex::TryLock () noexcept
{
    return TryAcquireSRWLockExclusive( &Mutex ) != 0;
}

Lumen::Bool Lumen::HAL::FWindowsSharedMutex::TryLockShared () noexcept
{
    return TryAcquireSRWLockShared( &Mutex ) != 0;
}
