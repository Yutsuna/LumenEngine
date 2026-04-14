/**
 * @file Mutex.inl
 * @brief Inline implementation for the FMutex class.
 */

#include "Thread/Mutex.hpp"

namespace LumenEngine
{

template <typename MutexType> TLockGuard<MutexType>::TLockGuard( MutexType &InMutex ) noexcept : Mutex( InMutex )
{
    Mutex.Lock();
}

template <typename MutexType> TLockGuard<MutexType>::~TLockGuard() noexcept
{
    Mutex.Unlock();
}

} // namespace LumenEngine