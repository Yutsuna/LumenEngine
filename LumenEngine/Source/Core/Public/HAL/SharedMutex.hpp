/**
 * @file SharedMutex.hpp
 * @brief Shared (readers-writer) mutex for LumenEngine thread synchronization.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "NonCopyable.hpp"
#include "NonMovable.hpp"

namespace LumenEngine
{

/**
 * @class FSharedMutex
 * @brief A slim, allocation-free readers-writer spin-mutex.
 */
class LUMEN_ENGINE_API FSharedMutex final : public FNonCopyable, public FNonMovable
{
public:

    FSharedMutex () noexcept  = default;
    ~FSharedMutex () noexcept = default;

public:

    /**
     * @brief Acquires the exclusive (write) lock.
     * @details Blocks until all readers have drained and no other writer holds
     *          the lock.  Uses `atomic::wait` for efficient suspending spin.
     */
    void Lock () noexcept;

    /**
     * @brief Attempts to acquire the exclusive (write) lock without blocking.
     * @returns `true`  if the lock was acquired (no readers, no other writer).
     * @returns `false` if the lock is currently unavailable.
     */
    [[nodiscard]] Bool TryLock () noexcept;

    /**
     * @brief Releases the exclusive (write) lock and wakes waiting threads.
     * @pre   The calling thread must currently hold the exclusive lock.
     */
    void Unlock () noexcept;

    /**
     * @brief Acquires a shared (read) lock.
     * @details Multiple threads may hold the shared lock concurrently.  Blocks
     *          while a writer holds or is waiting for the exclusive lock.
     */
    void LockShared () noexcept;

    /**
     * @brief Attempts to acquire the shared (read) lock without blocking.
     * @returns `true`  if the shared lock was acquired (no active writer).
     * @returns `false` if a writer is currently active.
     */
    [[nodiscard]] Bool TryLockShared () noexcept;

    /**
     * @brief Releases a shared (read) lock.
     * @pre   The calling thread must currently hold a shared lock.
     */
    void UnlockShared () noexcept;

public:

    /** @returns `true` if the exclusive lock is currently held. */
    [[nodiscard]] Bool IsWriteLocked () const noexcept;

    /** @returns The number of threads currently holding the shared lock. */
    [[nodiscard]] UInt32 SharedLockCount () const noexcept;

private:

    /** @brief Spins (with backoff + wait) until WriterFlag equals InExpected. */
    void WaitForWriterFlag ( UInt32 InExpected ) noexcept;

    /** @brief Spins (with backoff + wait) until ReaderCount reaches zero. */
    void WaitForReadersTodrain () noexcept;

private:

    /**
     * @brief Non-zero while a writer is waiting for or holds the exclusive lock.
     * @details `0` = unlocked, `1` = writer active.
     */
    TAtomic<UInt32> WriterFlag{ 0U };

    /**
     * @brief Count of threads currently holding a shared (read) lock.
     */
    TAtomic<UInt32> ReaderCount{ 0U };
};

/**
 * @class TSharedLock
 * @brief RAII scope-guard for the shared (read) lock of a readers-writer mutex.
 *
 * @details Acquires the shared lock on construction and releases it on
 *          destruction.  Non-copyable and non-movable.
 *
 * @tparam MutexType  A type exposing `LockShared()` and `UnlockShared()`.
 *                    Typically `FSharedMutex`.
 * @example
 * ```cpp
 * FSharedMutex Mtx;
 * {
 *     TSharedLock< LumenEngine::FSharedMutex > Guard( Mtx );   // shared lock acquired
 *     // … read shared state …
 * }                                    // shared lock released
 * ```
 */
template <typename MutexType> class TSharedLock final : public FNonCopyable, public FNonMovable
{
public:

    /**
     * @brief Constructs the guard and acquires the shared lock.
     * @param InMutex  The mutex to lock.  Must outlive this guard.
     */
    explicit TSharedLock ( MutexType &InMutex ) noexcept;

    /** @brief Releases the shared lock. */
    ~TSharedLock () noexcept;

private:

    MutexType &Mutex;
};

/**
 * @class TSharedUniqueLock
 * @brief Move-only RAII wrapper for the exclusive (write) lock of a
 *        readers-writer mutex.
 *
 * @details Mirrors `TUniqueLock` for `FSharedMutex` exclusive ownership.
 *          Supports deferred acquisition, `TryLock`, and ownership transfer
 *          via move semantics.
 *
 * @tparam MutexType  A type exposing `Lock()`, `TryLock()`, and `Unlock()`.
 *                    Typically `FSharedMutex`.
 *
 * @example
 * ```cpp
 * FSharedMutex Mtx;
 * {
 *     TSharedUniqueLock WriteLock( Mtx );   // exclusive lock acquired
 *     // … mutate shared state …
 * }                                         // exclusive lock released
 * ```
 */
template <typename MutexType> class TSharedUniqueLock final : public FNonCopyable
{
public:

    TSharedUniqueLock () noexcept = default;

    /**
     * @brief Constructs the wrapper and immediately acquires the exclusive lock.
     * @param InMutex  The mutex to lock.  Must outlive this wrapper.
     */
    explicit TSharedUniqueLock ( MutexType &InMutex ) noexcept;

    /** @brief Releases the exclusive lock if currently held. */
    ~TSharedUniqueLock () noexcept;

    TSharedUniqueLock ( TSharedUniqueLock &&InOther ) noexcept;
    TSharedUniqueLock &operator=( TSharedUniqueLock &&InOther ) noexcept;

public:

    /** @brief Acquires the exclusive lock.  No-op if the mutex pointer is null. */
    void Lock () noexcept;

    /** @brief Releases the exclusive lock.  No-op if not currently locked. */
    void Unlock () noexcept;

    /**
     * @brief Attempts to acquire the exclusive lock without blocking.
     * @returns `true` if the lock was acquired, `false` otherwise.
     */
    [[nodiscard]] Bool TryLock () noexcept;

    /**
     * @brief Releases ownership of the mutex without unlocking it.
     * @returns A pointer to the managed mutex, or `nullptr` if none.
     * @note After this call the wrapper no longer manages any mutex.
     */
    [[nodiscard]] MutexType *Release () noexcept;

    /** @returns `true` if the exclusive lock is currently held by this wrapper. */
    [[nodiscard]] Bool IsLocked () const noexcept;

    /** @brief Implicit conversion to `Bool`; equivalent to `IsLocked()`. */
    [[nodiscard]] explicit operator Bool () const noexcept;

private:

    MutexType *Mutex = nullptr;
    Bool bIsLocked   = false;
};

} // namespace LumenEngine

#include "Inline/SharedMutex.inl"