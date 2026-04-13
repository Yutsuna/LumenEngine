/**
 * @file Mutex.inl
 * @brief Inline implementation for the FMutex class.
 */

#include "Thread/Mutex.hpp"

template <typename MutexType> LumenEngine::TLockGuard<MutexType>::TLockGuard( MutexType &InMutex ) noexcept : Mutex( InMutex )
{
    Mutex.Lock();
}

template <typename MutexType> LumenEngine::TLockGuard<MutexType>::~TLockGuard() noexcept
{
    Mutex.Unlock();
}