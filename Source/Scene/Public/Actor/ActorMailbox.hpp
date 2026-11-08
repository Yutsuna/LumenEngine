/**
 * @file ActorMailbox.hpp
 * @brief Actor mailbox definition.
 */

#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

#include <Templates/Optional.hpp>

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Thread/SPMCQueue.hpp>

#include <cassert>

namespace Lumen
{

/**
 * @class FMailBox
 * @brief Wait-free MPSC (multi-producer, single-consumer) mailbox.

 * @details Implements the Dmitry Vyukov intrusive MPSC queue with an internal
 *          lock-free free-list to prevent per-message allocation bottlenecks.
 */
class SCENE_EXPORT FMailBox final : public FNonCopyable, public FNonMovable
{
private:

    /**
     * @struct FNode
     * @brief Intrusive linked-list node for the Vyukov MPSC queue.
     */
    struct alignas( 64 ) FNode
    {
        TAtomic<FNode *> Next = { nullptr };
        TOptional<FMessage> Message;
    };

public:

    FMailBox () noexcept;
    ~FMailBox () noexcept;

public:

    /**
     * @brief Enqueues a message. Safe to call from any thread simultaneously.
     * @param InMessage The message to send.
     */
    void Push ( const FMessage &InMessage ) noexcept;

    /**
     * @brief Attempts to dequeue the front message.
     * @return The message if one is available, TOptional<FMessage>{} otherwise.
     * @note Must be called from a single consumer thread only.
     */
    [[nodiscard]] TOptional<FMessage> Pop () noexcept;

    /**
     * @brief Pre-allocates nodes in the free-list pool.
     * @param InCapacity The number of nodes to pre-allocate.
     * @note Should be called from the consumer thread or during initialization.
     */
    void Reserve ( USize InCapacity ) noexcept;

    /**
     * @brief Freezes the tick boundary at the current tail.
     * @note Consumer-only. Called by FWorld::Tick before the parallel phase.
     */
    void BeginTick () noexcept;

    /**
     * @brief Clears the tick boundary. Pop resumes processing all nodes.
     * @note Consumer-only. Called by FWorld::Tick after the parallel phase.
     */
    void EndTick () noexcept;

    /**
     * @brief Returns true if no messages are pending.
     * @note May transiently return true during a producer gap. Single-consumer only.
     */
    [[nodiscard]] Bool IsEmpty () const noexcept;

private:

    /**
     * @brief Retires a previously consumed head node.
     * Resets its message and returns it to the free pool, or deletes it if the pool is full.
     */
    void RecycleHead ( FNode *InOldHead ) noexcept;

private:

    /** @brief Lock-free pool of nodes to eliminate dynamic allocations in hot paths. */
    alignas( 64 ) Thread::TSPMCQueue<FNode *> FreeNodes;

    /** @brief Stub sentinel — Head always points to the node *before* the first real message. */
    alignas( 64 ) FNode Stub = {};

    /** @brief Head: the node whose Next holds the next message to consume. Consumer-owned. */
    alignas( 64 ) FNode *Head = nullptr;

    /** @brief Tail: last node inserted. Written by producers via atomic exchange. */
    alignas( 64 ) TAtomic<FNode *> Tail = { nullptr };

    /** @brief Last node enqueued before the parallel phase; nullptr = no fence. */
    FNode *Boundary = nullptr;

}; // class FMailBox

} // namespace Lumen
