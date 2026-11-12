/**
 * @file WindowsRecursiveSharedMutex.inl
 * @brief Recursive mutex implementation.
 */

#pragma once

#include <HAL/Windows/WindowsRecursiveSharedMutex.hpp>

/**
 * Public
 */

Lumen::HAL::FWindowsRecursiveSharedMutex::FWindowsRecursiveSharedMutex () noexcept
{
    static_cast<void>( InitializeCriticalSectionAndSpinCount( &Mutex, GSpinCount ) );
}

Lumen::HAL::FWindowsRecursiveSharedMutex::~FWindowsRecursiveSharedMutex () noexcept
{
    DeleteCriticalSection( &Mutex );
}

void Lumen::HAL::FWindowsRecursiveSharedMutex::Lock () noexcept
{
    EnterCriticalSection( &Mutex );
}

void Lumen::HAL::FWindowsRecursiveSharedMutex::Unlock () noexcept
{
    LeaveCriticalSection( &Mutex );
}

Lumen::Bool Lumen::HAL::FWindowsRecursiveSharedMutex::TryLock () noexcept
{
    return TryEnterCriticalSection( &Mutex ) != 0;
}
