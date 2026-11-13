/**
 * @file ConditionVariable.hpp
 * @brief Condition variable template class.
 */

#pragma once

#include <CoreExport.hpp>

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Concepts/IsLockable.hpp>

namespace Lumen
{

/**
 * @class FConditionVariable
 * @brief Futex-based condition variable.
 * @details The predicate must be modified under the same lock that is passed to Wait (or with
 *          equivalent synchronization), and NotifyOne/NotifyAll must be called after the state
 *          change. The sequence counter is sampled while the lock is held, which rules out lost
 *          wake-ups under that protocol.
 */
class CORE_EXPORT FConditionVariable final : public FNonCopyable, public FNonMovable
{
public:

    FConditionVariable () noexcept  = default;
    ~FConditionVariable () noexcept = default;

public:

    /** @brief Wakes up at least one waiting thread. */
    LUMEN_FORCE_INLINE void NotifyOne () noexcept;

    /** @brief Wakes up every waiting thread */
    LUMEN_FORCE_INLINE void NotifyAll () noexcept;

public:

    /** @brief Sleeps until notified */
    template <Concepts::CIsLockable TLockType> void Wait ( TLockType &InLock ) noexcept;

    /** @brief Sleeps until InPredicate() */
    template <Concepts::CIsLockable TLockType, class TPredicate>
    void Wait ( TLockType &InLock, TPredicate &&InPredicate ) noexcept;

    /** @brief Sleeps until notified with a relative timeout */
    template <Concepts::CIsLockable TLockType>
    [[nodiscard]] Bool WaitFor ( TLockType &InLock, Float64 InTimeoutSeconds ) noexcept;

    /** @brief Sleeps until InPredicate() with a relative timeout */
    template <Concepts::CIsLockable TLockType, class TPredicate>
    [[nodiscard]] Bool WaitFor ( TLockType &InLock, Float64 InTimeoutSeconds, TPredicate &&InPredicate ) noexcept;

    /** @brief Sleeps until notified with an absolute deadline */
    template <Concepts::CIsLockable TLockType>
    [[nodiscard]] Bool WaitUntil ( TLockType &InLock, Float64 InDeadlineSeconds ) noexcept;

    /** @brief Sleeps until InPredicate() with an absolute deadline */
    template <Concepts::CIsLockable TLockType, class TPredicate>
    [[nodiscard]] Bool WaitUntil ( TLockType &InLock, Float64 InDeadlineSeconds, TPredicate &&InPredicate ) noexcept;

private:

    /** @brief Incremented by every notification. */
    TAtomic<UInt32> Sequence{ 0U };
};

} // namespace Lumen

#include "Inline/ConditionVariable.inl"
