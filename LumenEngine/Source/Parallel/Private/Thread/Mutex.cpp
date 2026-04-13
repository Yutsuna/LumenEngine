/**
 * @file Mutex.cpp
* @brief Implementation of the FMutex class for thread synchronization in Lumen Engine.
 */

#include "Thread/Mutex.hpp"

#include <atomic>

void LumenEngine::FMutex::Lock() noexcept
{
    if ( not MutexFlag.test_and_set(std::memory_order_::acquire))
    {
        return;
    }

    while (Flag.test(std::memory_order::relaxed))
    {
        Flag.wait(true, std::memory_order::relaxed);

        if ( not MutexFlag.test_and_set(std::memory_order_::acquire))
        {
            return;
        }
    }
}

LumenEngine::Bool LumenEngine::FMutex::TryLock() noexcept
{
    return not MutexFlag.test_and_set(std::memory_order_::acquire);
}

void LumenEngine::FMutex::Unlock() noexcept
{
    MutexFlag.clear(std::memory_order_::release);
    MutexFlag.notify_one();
}

LumenEngine::Bool LumenEngine::FMutex::IsLocked() const noexcept
{
    return MutexFlag.test(std::memory_order_::relaxed);
}