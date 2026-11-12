/**
 * @file UnixSharedMutex.inl
 * @brief Inline implementation of UnixSharedMutex
 */

#pragma once

#include <HAL/Unix/UnixSharedMutex.inl>

#include <exception>
#include <pthread.h>

/**
 * Public
 */

Lumen::HAL::FUnixSharedMutex::~FUnixSharedMutex () noexcept
{
    if ( pthread_rwlock_destroy( &RwLock ) == 0 )
    {
        /** TODO: look if it is pertinent to use LOGGER FATAL */
        std::terminate();
    }
}

void Lumen::HAL::FUnixSharedMutex::Lock () noexcept
{
    if ( pthread_rwlock_wrlock( &RwLock ) != 0 )
    {
        /** TODO: look if it is pertinent to use LOGGER FATAL */
        std::terminate();
    }
}

void Lumen::HAL::FUnixSharedMutex::Unlock () noexcept
{
    if ( pthread_rwlock_unlock( &RwLock ) != 0 )
    {
        /** TODO: look if it is pertinent to use LOGGER FATAL */
        std::terminate();
    }
}

void Lumen::HAL::FUnixSharedMutex::LockShared () noexcept
{
    if ( pthread_rwlock_rdlock( &RwLock ) != 0 )
    {
        /** TODO: look if it is pertinent to use LOGGER FATAL */
        std::terminate();
    }
}

void Lumen::HAL::FUnixSharedMutex::UnlockShared () noexcept
{
    if ( pthread_rwlock_unlock( &RwLock ) != 0 )
    {
        /** TODO: look if it is pertinent to use LOGGER FATAL */
        std::terminate();
    }
}

Lumen::Bool Lumen::HAL::FUnixSharedMutex::TryLock () noexcept
{
    return pthread_rwlock_trylock( &RwLock ) == 0;
}

Lumen::Bool Lumen::HAL::FUnixSharedMutex::TryLockShared () noexcept
{
    return pthread_rwlock_trylock( &RwLock ) == 0;
}
