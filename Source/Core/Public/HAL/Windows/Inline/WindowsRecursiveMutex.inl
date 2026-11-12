/**
 * @file WindowsRecursiveMutex.inl
 * @brief Recursive mutex implementation.
 */

#pragma once

#include <HAL/Windows/WindowsRecursiveMutex.hpp>

/**
 * Public
 */

Lumen::HAL::FWindowsRecursiveMutex::FWindowsRecursiveMutex () noexcept
{
    static_cast<void>( InitializeCriticalSectionAndSpinCount( &Mutex, GSpinCount ) );
}

Lumen::HAL::FWindowsRecursiveMutex::~FWindowsRecursiveMutex () noexcept
{
    DeleteCriticalSection( &Mutex );
}

void Lumen::HAL::FWindowsRecursiveMutex::Lock () noexcept
{
    EnterCriticalSection( &Mutex );
}

void Lumen::HAL::FWindowsRecursiveMutex::Unlock () noexcept
{
    LeaveCriticalSection( &Mutex );
}

Lumen::Bool Lumen::HAL::FWindowsRecursiveMutex::TryLock () noexcept
{
    return TryEnterCriticalSection( &Mutex ) != 0;
}
