/**
 * @file SpinLock.hpp
 * @brief Busy-wait spin-lock for Lumen thread synchronization.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>
#include <CoreExport.hpp>

#include <HAL/Interface/IMutex.hpp>

namespace Lumen
{

namespace HAL
{

    /**
     * @class FSpinLock
     * @brief TTAS spin-lock that never blocks the calling thread.
     */
    class CORE_EXPORT FSpinLock final : public FNonCopyable, public FNonMovable, public Internal::IMutex
    {

    public:

        FSpinLock () noexcept           = default;
        ~FSpinLock () noexcept override = default;

    public:

        /** @briefe Spins with platform relax hint until the lock is acquired. */
        void Lock () noexcept override;

        /** @brief Attempts to acquire the Lock without spinning. */
        [[nodiscard]] Bool TryLock () noexcept override;

        /** @brief Releases the Lock. */
        void Unlock () noexcept override;

        /** @brief Checks if the Lock is currently held. */
        [[nodiscard]] Bool IsLocked () const noexcept override;

        /** @brief Checks if the Lock is currently held. */
        [[nodiscard]] explicit operator Bool () const noexcept override;

    private:

        /** @brief The underlying atomic flag used for locking. */
        FAtomicFlag MutexFlag = ATOMIC_FLAG_INIT;
    };

    /**
     * @class FSpinGuard
     * @brief RAII guard for FSpinLock that automatically releases the lock when destroyed.
     */
    class CORE_EXPORT FSpinGuard : public FNonCopyable, public FNonMovable
    {

    public:

        explicit FSpinGuard ( FSpinLock &InLock ) noexcept;
        ~FSpinGuard () noexcept;

    private:

        FSpinLock &Lock;
    };

} // namespace HAL

} // namespace Lumen
