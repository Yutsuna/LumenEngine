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

#include <Thread/SPMCQueue.hpp>

#include <cassert>

namespace Lumen
{

///
/// @class FMailBox
/// @brief Wait-free Dmitry Vyukov MPSC queue
///
class SCENE_EXPORT FMailBox final : public FNonCopyable, public FNonMovable
{
private:

    ///
    /// @struct FNode
    /// @brief Intrusive linked-list node for the Vyukov MPSC queue.
    ///
    struct alignas( 64 ) FNode
    {
        TAtomic<FNode *> Next = { nullptr };
        TOptional<FMessage> Message;
    };

public:

    FMailBox () noexcept;
    ~FMailBox () noexcept;

public:

    ///
    /// @brief Enqueues a message. Safe to call from any thread simultaneously.
    /// @param InMessage The message to send.
    ///
    void Push ( const FMessage &InMessage ) noexcept;

    ///
    /// @brief Attempts to dequeue the front message.
    /// @return The message if one is available, TOptional<FMessage>{} otherwise.
    /// @note Must be called from a single consumer thread only.
    ///
    [[nodiscard]] TOptional<FMessage> Pop () noexcept;

    ///
    /// @brief Pre-allocates nodes in the free-list pool.
    /// @param InCapacity The number of nodes to pre-allocate.
    /// @note Should be called from the consumer thread or during initialization.
    ///
    void Reserve ( USize InCapacity ) noexcept;

    ///
    /// @brief Returns true if no messages are pending.
    /// @note May transiently return true during a producer gap. Single-consumer only.
    ///
    [[nodiscard]] Bool IsEmpty () const noexcept;

private:

    /// @brief Lock-free pool of nodes to eliminate dynamic allocations in hot paths.
    alignas( 64 ) Thread::TSPMCQueue<FNode *> FreeNodes;

    /// @brief Stub sentinel — Head always points to the node *before* the first real message.
    alignas( 64 ) FNode Stub = {};

    /// @brief Head: the node whose Next holds the next message to consume. Consumer-owned.
    alignas( 64 ) FNode *Head = nullptr;

    /// @brief Tail: last node inserted. Written by producers via atomic exchange.
    alignas( 64 ) TAtomic<FNode *> Tail = { nullptr };
};

} // namespace Lumen
