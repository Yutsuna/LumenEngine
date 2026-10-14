///
/// @file Mutex.cpp
/// @brief Implementation of the FMutex class for thread synchronization in Lumen Engine.
///

#include <HAL/Mutex.hpp>

#include <atomic>

void Lumen::FMutex::Lock() noexcept
{
    while (MutexFlag.test_and_set(std::memory_order::acquire))
    {
        MutexFlag.wait(true, std::memory_order::relaxed);
    }
}

Lumen::Bool Lumen::FMutex::TryLock() noexcept
{
    return not MutexFlag.test_and_set(std::memory_order::acquire);
}

void Lumen::FMutex::Unlock() noexcept
{
    MutexFlag.clear(std::memory_order::release);
    MutexFlag.notify_one();
}

Lumen::Bool Lumen::FMutex::IsLocked() const noexcept
{
    return MutexFlag.test(std::memory_order::relaxed);
}

Lumen::FMutex::operator Bool() const noexcept
{
    return IsLocked();
}