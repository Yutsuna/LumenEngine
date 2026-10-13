///
/// @file Mutex.hpp
/// @brief Mutex class definition for thread synchronization.
///

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/Interface/IMutex.hpp>

namespace Lumen
{

///
/// @class FMutex
/// @brief A highly optimized lightweight spin-mutex.
///
class FMutex final : public FNonCopyable, public FNonMovable, public Internal::IMutex
{
public:

    FMutex () noexcept           = default;
    ~FMutex () noexcept override = default;

public:

    /// @brief Acquires the mutex. Spins & then waits if the mutex is unavailable
    void Lock () noexcept override;

    /// @brief Attempts to acquire the mutex without blocking.
    [[nodiscard]] Bool TryLock () noexcept override;

    /// @brief Releases the mutex.
    void Unlock () noexcept override;

    /// @brief Checks if the mutex is locked.
    [[nodiscard]] Bool IsLocked () const noexcept override;

    [[nodiscard]] explicit operator Bool () const noexcept override;

private:

    /// @brief The underlying atomic flag used for locking
    FAtomicFlag MutexFlag = ATOMIC_FLAG_INIT;
};

template <typename MutexType> class TLockGuard final : public FNonCopyable, public FNonMovable
{
public:

    explicit TLockGuard ( MutexType &InMutex ) noexcept;
    ~TLockGuard () noexcept;

private:

    MutexType &Mutex;
};

///
/// @class TUniqueLock
/// @brief A move-only mutex ownership wrapper.
///
template <typename MutexType> class TUniqueLock final : public FNonCopyable, public Internal::IMutex
{
public:

    TUniqueLock () noexcept = default;
    explicit TUniqueLock ( MutexType &InMutex ) noexcept;
    ~TUniqueLock () noexcept override;

    TUniqueLock ( TUniqueLock &&InOther ) noexcept;
    TUniqueLock &operator=( TUniqueLock &&InOther ) noexcept;

public:

    void Lock () noexcept override;
    void Unlock () noexcept override;
    [[nodiscard]] Bool TryLock () noexcept override;

    /// @brief Releases the mutex ownership without unlocking it.
    MutexType *Release () noexcept;

    [[nodiscard]] Bool IsLocked () const noexcept override;
    [[nodiscard]] explicit operator Bool () const noexcept override;

private:

    MutexType *Mutex = nullptr;
    Bool bIsLocked   = false;
};
} // namespace Lumen

#include "Inline/Mutex.inl"