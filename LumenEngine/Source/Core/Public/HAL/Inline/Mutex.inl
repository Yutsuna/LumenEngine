/**
 * @file Mutex.inl
 * @brief Inline implementation for the FMutex class.
 */

#include "HAL/Mutex.hpp"

namespace LumenEngine
{

template <typename MutexType> TLockGuard<MutexType>::TLockGuard ( MutexType &InMutex ) noexcept : Mutex( InMutex )
{
    Mutex.Lock();
}

template <typename MutexType> TLockGuard<MutexType>::~TLockGuard () noexcept
{
    Mutex.Unlock();
}

/**
 * TUniqueLock Implementation
 */

template <typename MutexType> TUniqueLock<MutexType>::TUniqueLock ( MutexType &InMutex ) noexcept : Mutex( &InMutex )
{
    Lock();
}

template <typename MutexType> TUniqueLock<MutexType>::~TUniqueLock () noexcept
{
    if ( bIsLocked )
    {
        Unlock();
    }
}

template <typename MutexType> TUniqueLock<MutexType>::TUniqueLock ( TUniqueLock &&InOther ) noexcept : Mutex( InOther.Mutex ), bIsLocked( InOther.bIsLocked )
{
    InOther.Mutex     = nullptr;
    InOther.bIsLocked = false;
}

template <typename MutexType> TUniqueLock<MutexType> &TUniqueLock<MutexType>::operator=( TUniqueLock &&InOther ) noexcept
{
    if ( this != &InOther )
    {
        if ( bIsLocked )
        {
            Unlock();
        }

        Mutex             = InOther.Mutex;
        bIsLocked         = InOther.bIsLocked;
        InOther.Mutex     = nullptr;
        InOther.bIsLocked = false;
    }

    return *this;
}

template <typename MutexType> void TUniqueLock<MutexType>::Lock () noexcept
{
    if ( Mutex )
    {
        Mutex->Lock();
        bIsLocked = true;
    }
}

template <typename MutexType> void TUniqueLock<MutexType>::Unlock () noexcept
{
    if ( Mutex and bIsLocked )
    {
        Mutex->Unlock();
        bIsLocked = false;
    }
}

template <typename MutexType> Bool TUniqueLock<MutexType>::TryLock () noexcept
{
    if ( Mutex )
    {
        bIsLocked = Mutex->TryLock();
        return bIsLocked;
    }

    return false;
}

template <typename MutexType> MutexType *TUniqueLock<MutexType>::Release () noexcept
{
    MutexType *ReleasedMutex = Mutex;
    Mutex                    = nullptr;
    bIsLocked                = false;

    return ReleasedMutex;
}

template <typename MutexType> Bool TUniqueLock<MutexType>::IsLocked () const noexcept
{
    return bIsLocked;
}

template <typename MutexType> TUniqueLock<MutexType>::operator bool () const noexcept
{
    return IsLocked();
}

} // namespace LumenEngine