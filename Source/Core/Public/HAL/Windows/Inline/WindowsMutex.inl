/**
 * @file WindowsMutex.inl
 * @brief Non-recursive mutex implementation
 */

#pragma once

#include <HAL/Windows/WindowsMutex.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )
    #if not defined( LUMEN_CLANG_TIDY )
        #error "WindowsMutex.inl is only supported on Windows"
    #endif
#else

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

#endif
