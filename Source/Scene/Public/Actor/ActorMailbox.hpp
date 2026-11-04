///
/// @file ActorMailbox.hpp
/// @brief Actor mailbox definition for parallel processing in Lumen Engine.
///

#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

#include <Templates/Optional.hpp>

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/Mutex.hpp>
#include <Thread/SPMCQueue.hpp>

namespace Lumen
{

///
/// @class FMailBox
/// @brief MPSC queue with lock-free publication, node pooling, and a potentially blocking allocation fallback.
/// @details Push is multi-producer. Pop and IsEmpty are single-consumer operations. Reserve and Pop are the only
/// producers of free nodes and must never run concurrently. Producer-thread handoffs require external ordering.
///
class SCENE_EXPORT FMailBox final : public FNonCopyable, public FNonMovable
{
private:

    struct alignas( 64 ) FNode
    {
        TAtomic<FNode *> Next = { nullptr };
        TOptional<FMessage> Message;
    };

    static_assert( TAtomic<FNode *>::is_always_lock_free );
    static_assert( TAtomic<UInt32>::is_always_lock_free );
    static_assert( TAtomic<USize>::is_always_lock_free );

public:

    static constexpr USize PoolCapacity = 256ULL;

    explicit FMailBox ( USize InMaxMessages = 0ULL, USize InPreallocatedNodes = 0ULL ) noexcept;
    ~FMailBox () noexcept;

public:

    /// @brief Enqueues a message, or reports why delivery did not occur.
    /// @note Node publication is lock-free; the allocation fallback may block.
    [[nodiscard]] EEnqueueResult Push ( const FMessage &InMessage ) noexcept;

    /// @brief Attempts to dequeue the front message. Single-consumer only.
    /// @pre Must not run concurrently with Reserve, which also produces free nodes.
    [[nodiscard]] TOptional<FMessage> Pop () noexcept;

    /// @brief Pre-allocates up to PoolCapacity free nodes, coordinated with Close.
    /// @note For allocation-free steady-state delivery, preallocate during initialization and bound capacity accordingly.
    /// @pre Call before publication or from the consumer thread. Never concurrent with Pop or another Reserve; serialize
    /// any producer-thread handoff externally. The constructor's initial reservation precedes mailbox publication.
    void Reserve ( USize InCapacity ) noexcept;

    /// @brief Rejects new operations, waits for active ones, and discards all pending messages.
    /// @note The owner must stop the consumer before destruction. Close itself is safe to call repeatedly.
    void Close () noexcept;

    /// @brief Returns true if no messages are currently visible to the consumer, or if the mailbox is closed.
    /// @note May transiently return true during a producer gap. Single-consumer and coordinated with Close.
    [[nodiscard]] Bool IsEmpty () const noexcept;

    [[nodiscard]] Bool IsClosed () const noexcept;

private:

    [[nodiscard]] Bool TryBeginOperation () const noexcept;
    void EndOperation () const noexcept;

private:

    static constexpr UInt32 ClosedFlag         = 1U;
    static constexpr UInt32 OperationIncrement = 2U;

    /// The bounded SPMC buffer holds pointers; nodes are allocated separately by Reserve.
    alignas( 64 ) Thread::TSPMCQueue<FNode *> FreeNodes;

    alignas( 64 ) FNode Stub            = {};
    FNode *Head                         = nullptr;
    alignas( 64 ) TAtomic<FNode *> Tail = { nullptr };

    alignas( 64 ) mutable TAtomic<UInt32> OperationState = { 0U };
    TAtomic<USize> MessageCount                          = { 0ULL };
    USize MaxMessages                                    = 0ULL;

    FMutex CloseMutex;
    Bool bResourcesReleased = false;
};

} // namespace Lumen
