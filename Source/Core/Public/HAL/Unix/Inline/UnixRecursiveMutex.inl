/**
 * @file UnixRecursiveMutex.inl
 * @brief Inline implementation of FUnixRecursiveSharedMutex.
 */

#pragma once

#include <HAL/PlatformAtomic.hpp>
#include <HAL/Unix/UnixRecursiveMutex.hpp>

#include <pthread.h>

/**
 * Public
 */

Lumen::HAL::FUnixRecursiveSharedMutex::FUnixRecursiveSharedMutex () noexcept : Mutex( PTHREAD_MUTEX_INITIALIZER )
{
    pthread_mutexattr_t Attributes;

    if ( pthread_mutexattr_init( &Attributes ) != 0 ) [[unlikely]]
    {
        return;
    }

    if ( pthread_mutexattr_settype( &Attributes, PTHREAD_MUTEX_RECURSIVE ) != 0 ) [[unlikely]]
    {
        return;
    }

    if ( pthread_mutex_init( &Mutex, &Attributes ) != 0 ) [[unlikely]]
    {
        return;
    }

    pthread_mutexattr_destroy( &Attributes );
}

Lumen::HAL::FUnixRecursiveSharedMutex::~FUnixRecursiveSharedMutex () noexcept
{
    pthread_mutex_destroy( &Mutex );
}

void Lumen::HAL::FUnixRecursiveSharedMutex::Lock () noexcept
{
    if ( pthread_mutex_trylock( &Mutex ) == 0 )
    {
        return;
    }

    for ( UInt32 Round = 1U; Round <= GSpinRounds; ++Round )
    {
        for ( UInt32 Pause = 0U; Pause < ( 1U << Round ); ++Pause )
        {
            CpuRelax();
        }

        if ( pthread_mutex_trylock( &Mutex ) == 0 )
        {
            return;
        }
    }

    if ( pthread_mutex_lock( &Mutex ) != 0 ) [[unlikely]]
    {
        return;
    }
}

Lumen::Bool Lumen::HAL::FUnixRecursiveSharedMutex::TryLock () noexcept
{
    return pthread_mutex_trylock( &Mutex ) == 0;
}

void Lumen::HAL::FUnixRecursiveSharedMutex::Unlock () noexcept
{
    if ( pthread_mutex_unlock( &Mutex ) != 0 ) [[unlikely]]
    {
        return;
    }
}
