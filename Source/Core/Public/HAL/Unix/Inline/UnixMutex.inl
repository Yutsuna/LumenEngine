/**
 * @file UnixMutex.inl
 * @brief Inline implementation of Unix Mutex
 */

#pragma once

#include <HAL/Unix/UnixMutex.hpp>

/**
 * Public
 */

Lumen::HAL::FUnixMutex::~FUnixMutex () noexcept
{
    if ( pthread_mutex_destroy( &Mutex ) != 0 ) [[unlikely]]
    {
        return;
    }
}

void Lumen::HAL::FUnixMutex::Lock () noexcept
{
    if ( pthread_mutex_lock( &Mutex ) != 0 ) [[unlikely]]
    {
        return;
    }
}

void Lumen::HAL::FUnixMutex::Unlock () noexcept
{
    if ( pthread_mutex_unlock( &Mutex ) != 0 ) [[unlikely]]
    {
        return;
    }
}

Lumen::Bool Lumen::HAL::FUnixMutex::TryLock () noexcept
{
    return pthread_mutex_trylock( &Mutex ) == 0;
}
