/**
 * @file Mutex.cpp
 * @brief Implementation of the FMutex class for thread synchronization in Lumen Engine.
 */

#include "Thread/Mutex.hpp"

#include <atomic>

void LumenEngine::FMutex::Lock () noexcept
{
    if ( not MutexFlag.test_and_set( std::memory_order::acquire ) )
    {
        return;
    }

    while ( MutexFlag.test( std::memory_order::relaxed ) )
    {
        MutexFlag.wait( true, std::memory_order::relaxed );

        if ( not MutexFlag.test_and_set( std::memory_order::acquire ) )
        {
            return;
        }
    }
}

LumenEngine::Bool LumenEngine::FMutex::TryLock () noexcept
{
    return not MutexFlag.test_and_set( std::memory_order::acquire );
}

void LumenEngine::FMutex::Unlock () noexcept
{
    MutexFlag.clear( std::memory_order::release );
    MutexFlag.notify_one();
}

LumenEngine::Bool LumenEngine::FMutex::IsLocked () const noexcept
{
    return MutexFlag.test( std::memory_order::relaxed );
}