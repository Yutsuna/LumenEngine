/**
 * @file ActorMailbox.hpp
 * @brief Actor mailbox definition.
 */

#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorLifecycle.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/UniquePtr.hpp>

#include <Thread/MPSCQueue.hpp>

namespace Lumen
{

/** @brief Capacity used when a mailbox is constructed without an explicit descriptor. */
inline constexpr SCENE_EXPORT USize DefaultMailboxCapacity = 32ULL;

/**
 * @enum EActorMailboxError
 * @brief Structured failures of mailbox capacity control operations.
 */
enum class EActorMailboxError : UInt8
{
    InvalidCapacity,
    OutOfMemory,
    NotInitialized,
};

/**
 * @struct FMailBoxDesc
 * @brief Fixed capacity descriptor of an actor mailbox.
 * @details The capacity is allocated by the constructor or by Reserve. It never grows on a message path.
 */
struct SCENE_EXPORT FMailBoxDesc
{
    USize Capacity = DefaultMailboxCapacity;
};

/**
 * @class FMailBox
 * @brief Bounded wait-free MPSC (multi-producer, single-consumer) actor mailbox.
 *
 * @details Storage is provided by Core's Thread::TMPSCQueue<T>, whose nodes are allocated once by construction
 *          or by Reserve. Afterwards Push/TryPush/Pop/RecycleHead never allocate or release storage: a full
 *          mailbox returns a structured failure and preserves every accepted message. Producers run lock-free;
 *          the single consumer can briefly yield inside the Core free-list when a producer is preempted between
 *          claiming and publishing a recycled slot. Per-producer FIFO and linearization order are preserved;
 *          concurrent producers still have no deterministic total order.
 */
class SCENE_EXPORT FMailBox final : public FNonCopyable, public FNonMovable
{
public:

    /** @brief Constructs a mailbox with DefaultMailboxCapacity slots. */
    FMailBox () noexcept;

    /** @brief Constructs a mailbox with the capacity from InDesc. */
    explicit FMailBox ( const FMailBoxDesc &InDesc ) noexcept;

    ~FMailBox () noexcept = default;

public:

    /**
     * @brief Enqueues a message, keeping the legacy boolean contract.
     * @param InMessage The message to send.
     * @return True when the message was accepted, false when the mailbox is full or uninitialized.
     * @note A rejection increments the saturation counter and is logged; use TryPush for structured handling.
     */
    Bool Push ( const FMessage &InMessage ) noexcept;

    /**
     * @brief Enqueues a message with a structured saturation result.
     * @param InMessage The message to send.
     * @return Success, or EActorSendError::MailboxFull without modifying the mailbox or InMessage.
     * @note Safe for concurrent producers; a full mailbox leaves all previously accepted messages intact.
     */
    [[nodiscard]] TExpected<void, EActorSendError> TryPush ( const FMessage &InMessage ) noexcept;

    /**
     * @brief Attempts to dequeue the front message eligible for the current tick.
     * @return The message, or an empty optional when none is eligible or available.
     * @note Must be called from a single consumer thread only.
     */
    [[nodiscard]] TOptional<FMessage> Pop () noexcept;

    /**
     * @brief Initializes or enlarges the bounded storage. Serial control operation.
     * @param InCapacity Exact number of message slots; must be greater than zero.
     * @return Success when the mailbox can hold at least InCapacity messages, or a structured failure.
     * @note A smaller or equal existing capacity is a no-op. Enlarging transfers pending messages and must not
     *       race with any producer or consumer.
     */
    [[nodiscard]] TExpected<void, EActorMailboxError> Reserve ( USize InCapacity ) noexcept;

    /**
     * @brief Freezes the tick boundary at the number of messages already accepted.
     * @note Consumer-only. Called by FWorld::Tick before the parallel phase.
     * @note A producer preempted between publishing its node and publishing the accepted counter can have that
     *       message deferred by one additional fence. This is the same transient-gap caveat as the underlying
     *       MPSC queue: no message is lost or reordered, and the boundary remains a conservative serial point.
     */
    void BeginTick () noexcept;

    /**
     * @brief Clears the tick boundary. Pop resumes processing all accepted messages.
     * @note Consumer-only. Called by FWorld::Tick after the parallel phase.
     */
    void EndTick () noexcept;

    /**
     * @brief Returns true when no message is currently visible in the queue.
     * @note May transiently return true during a producer gap. Single-consumer only.
     */
    [[nodiscard]] Bool IsEmpty () const noexcept;

    /** @return The exact number of message slots, or zero when storage could not be allocated. */
    [[nodiscard]] USize GetCapacity () const noexcept;

    /** @return The number of messages rejected since construction. */
    [[nodiscard]] UInt64 GetSaturationCount () const noexcept;

    /** @return The number of messages currently accepted but not yet consumed. */
    [[nodiscard]] USize GetPendingCount () const noexcept;

private:

    /** @brief Sentinel stored in Boundary when no tick fence is active. */
    static constexpr USize NoBoundary = ~USize( 0ULL );

    [[nodiscard]] TExpected<void, EActorMailboxError> Initialize ( USize InCapacity ) noexcept;

private:

    TUniquePtr<Thread::TMPSCQueue<FMessage>> Queue;

    alignas( 64 ) TAtomic<USize> AcceptedCount    = { 0ULL };
    alignas( 64 ) TAtomic<USize> Boundary         = { NoBoundary };
    alignas( 64 ) TAtomic<UInt64> SaturationCount = { 0ULL };

    /** @brief Consumer-only cursor; equals the number of messages removed from the underlying queue. */
    USize ConsumedCount = 0ULL;

}; // class FMailBox

} // namespace Lumen
