/**
 * @file Mutex.hpp
 * @brief Mutex class definition for thread synchronization in Lumen Engine.
 */

#include "NonCopyable.hpp"
#include "NonMovable.hpp"

#include "CoreTypes.hpp"

#pragma once

namespace LumenEngine
{

/**
 * @class FMutex
 * @brief A highly optimized lightweight spin-mutex.
 * @details Uses C++23 atomic_flag notification for low-latency wakeups and
 * exponential backoff to reduce cache-line contention.
 */
class FMutex final : public FNonCopyable, public FNonMovable
{
public:

    FMutex () noexcept  = default;
    ~FMutex () noexcept = default;

public:

    /** @brief Acquires the mutex. Spins & then waits if the mutex is unavailable */
    void Lock () noexcept;

    /** @brief Attempts to acquire the mutex without blocking. */
    [[nodiscard]] Bool TryLock () noexcept;

    /** @brief Releases the mutex. */
    void Unlock () noexcept;

    /** @brief Checks if the mutex is locked. */
    [[nodiscard]] Bool IsLocked () const noexcept;

private:

    /** The underlying atomic flag used for locking */
    FAtomicFlag MutexFlag = ATOMIC_FLAG_INIT; /**< Atomic flag for lock state. */
};

template <typename MutexType> class TLockGuard final : public FNonCopyable, public FNonMovable
{
public:

    explicit TLockGuard ( MutexType &InMutex ) noexcept;
    ~TLockGuard () noexcept;

private:

    MutexType &Mutex;
};

/**
 * @class TUniqueLock
 * @brief A move-only mutex ownership wrapper.
 * @details Similar to std::unique_lock, but follows LumenEngine conventions.
 */
template <typename MutexType> class TUniqueLock final : public FNonCopyable
{
public:

    TUniqueLock () noexcept = default;
    explicit TUniqueLock ( MutexType &InMutex ) noexcept;
    ~TUniqueLock () noexcept;

    TUniqueLock ( TUniqueLock &&InOther ) noexcept;
    TUniqueLock &operator=( TUniqueLock &&InOther ) noexcept;

public:

    void Lock () noexcept;
    void Unlock () noexcept;
    [[nodiscard]] Bool TryLock () noexcept;

    /** @brief Releases the mutex ownership without unlocking it. */
    MutexType *Release () noexcept;

    [[nodiscard]] Bool IsLocked () const noexcept;
    [[nodiscard]] explicit operator bool () const noexcept;

private:

    MutexType *Mutex = nullptr;
    Bool bIsLocked   = false;
};

} // namespace LumenEngine

#include "Inline/Mutex.inl"