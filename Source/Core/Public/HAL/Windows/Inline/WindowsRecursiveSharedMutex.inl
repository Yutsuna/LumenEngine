/**
 * @file WindowsRecursiveSharedMutex.inl
 * @brief Recursive mutex implementation.
 */

#pragma once

#include <HAL/Windows/WindowsRecursiveSharedMutex.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )
    #if not defined( LUMEN_CLANG_TIDY )
        #error "WindowsRecursiveSharedMutex.inl is only supported on Windows"
    #endif
#else

/**
 * Public
 */

Lumen::HAL::FWindowsRecursiveSharedMutex::FWindowsRecursiveSharedMutex () noexcept : Mutex{}
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

#endif
