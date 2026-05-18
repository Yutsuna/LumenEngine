/**
 * @file SharedMutex.inl
 * @brief Inline template implementations for TSharedLock and TSharedUniqueLock.
 */

#pragma once

#include "HAL/SharedMutex.hpp"

namespace LumenEngine
{

/**
 * TSharedLock
 */

template <typename MutexType> TSharedLock<MutexType>::TSharedLock( MutexType &InMutex ) noexcept : Mutex( InMutex )
{
    Mutex.LockShared();
}

template <typename MutexType> TSharedLock<MutexType>::~TSharedLock() noexcept
{
    Mutex.UnlockShared();
}

/**
 * TSharedUniqueLock
 */

template <typename MutexType> TSharedUniqueLock<MutexType>::TSharedUniqueLock( MutexType &InMutex ) noexcept : Mutex( &InMutex )
{
    Lock();
}

template <typename MutexType> TSharedUniqueLock<MutexType>::~TSharedUniqueLock() noexcept
{
    if ( bIsLocked )
    {
        Unlock();
    }
}

template <typename MutexType>
TSharedUniqueLock<MutexType>::TSharedUniqueLock( TSharedUniqueLock &&InOther ) noexcept : Mutex( InOther.Mutex ), bIsLocked( InOther.bIsLocked )
{
    InOther.Mutex     = nullptr;
    InOther.bIsLocked = false;
}

template <typename MutexType> TSharedUniqueLock<MutexType> &TSharedUniqueLock<MutexType>::operator=( TSharedUniqueLock &&InOther ) noexcept
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

template <typename MutexType> void TSharedUniqueLock<MutexType>::Lock () noexcept
{
    if ( Mutex )
    {
        Mutex->Lock();
        bIsLocked = true;
    }
}

template <typename MutexType> void TSharedUniqueLock<MutexType>::Unlock () noexcept
{
    if ( Mutex and bIsLocked )
    {
        Mutex->Unlock();
        bIsLocked = false;
    }
}

template <typename MutexType> Bool TSharedUniqueLock<MutexType>::TryLock () noexcept
{
    if ( Mutex )
    {
        bIsLocked = Mutex->TryLock();
        return bIsLocked;
    }

    return false;
}

template <typename MutexType> MutexType *TSharedUniqueLock<MutexType>::Release () noexcept
{
    MutexType *ReleasedMutex = Mutex;
    Mutex                    = nullptr;
    bIsLocked                = false;

    return ReleasedMutex;
}

template <typename MutexType> Bool TSharedUniqueLock<MutexType>::IsLocked () const noexcept
{
    return bIsLocked;
}

template <typename MutexType> TSharedUniqueLock<MutexType>::operator Bool () const noexcept
{
    return IsLocked();
}

} // namespace LumenEngine
