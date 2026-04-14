/**
 * @file ActorMailbox.hpp
 * @brief Actor mailbox definition for parallel processing in Lumen Engine.
 */

#pragma once

#include "Actor/ActorMessage.hpp"
#include "Actor/ActorTypes.hpp"

#include "Container/Optional.hpp"

#include "CoreTypes.hpp"
#include "NonCopyable.hpp"
#include "NonMovable.hpp"

#include <cassert>

namespace LumenEngine
{

namespace Internal
{

    /**
     * @struct FMailBoxNode
     * @brief Intrusive linked-list node for the Vyukov MPSC queue.
     * @details Owns an optional message payload. The stub sentinel carries no message.
     */
    struct FMailBoxNode
    {
        TAtomic<FMailBoxNode *> Next = { nullptr };
        TOptional<FMessage> Message;
    };

} // namespace Internal

/**
 * @class FMailBox
 * @brief Wait-free MPSC (multi-producer, single-consumer) mailbox.
 *
 * @details Implements the Dmitry Vyukov intrusive MPSC queue.
 *
 *  Producers (any thread):
 *    - Allocate a node, store the message, atomic exchange on Tail → wait-free.
 *    - Write node.Next after exchange — may cause a transient gap.
 *
 *  Consumer (single thread only):
 *    - Pop() walks from Head forward via stub.Next.
 *    - Returns TOptional<FMessage>{} when the queue is empty OR during a transient gap.
 *    - Never blocks. Caller must handle empty returns gracefully.
 *
 *  Memory ordering:
 *    - Push: release on Tail exchange + relaxed store on Next.
 *    - Pop:  acquire on Next load to synchronise with the producer's release.
 */
class FMailBox final : public FNonCopyable, public FNonMovable
{
public:

    FMailBox () noexcept;
    ~FMailBox () noexcept;

public:

    /**
     * @brief Enqueues a message. Safe to call from any thread simultaneously.
     * @param InMessage The message to send. Moved into the internal node.
     */
    void Push ( FMessage InMessage ) noexcept;

    /**
     * @brief Attempts to dequeue the front message.
     * @return The message if one is available, TOptional<FMessage>{} otherwise.
     * @note Must be called from a single consumer thread only.
     */
    [[nodiscard]] TOptional<FMessage> Pop () noexcept;

    /**
     * @brief Returns true if no messages are pending.
     * @note May transiently return true during a producer gap. Single-consumer only.
     */
    [[nodiscard]] Bool IsEmpty () const noexcept;

private:

    using FNode = Internal::FMailBoxNode;

    /** @brief Stub sentinel — Head always points to the node *before* the first real message. */
    alignas( 64 ) FNode Stub = {};

    /** @brief Head: the node whose Next holds the next message to consume. Consumer-owned. */
    alignas( 64 ) FNode *Head = nullptr;

    /** @brief Tail: last node inserted. Written by producers via atomic exchange. */
    alignas( 64 ) TAtomic<FNode *> Tail = { nullptr };

}; // class FMailBox

} // namespace LumenEngine