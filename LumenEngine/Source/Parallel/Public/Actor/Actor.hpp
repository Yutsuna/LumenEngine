/**
 * @file Actor.hpp
 * @brief Actor base class for the LumenEngine parallel Actor Model.
 */

#pragma once

#include "Actor/ActorMailbox.hpp"
#include "Actor/ActorMessage.hpp"
#include "Actor/ActorTypes.hpp"

namespace LumenEngine
{

/**
 * @class AActor
 * @brief Abstract base for all actors.
 * @details Each actor owns a lock-free mailbox.
 *          The engine calls ProcessMailbox() from a worker thread
 *          Receive() is always single-threaded per instance.
 *          Actors communicate exclusively via AActorRef::Send(); no shared state.
 */
class AActor : public FNonCopyable
{
public:

    explicit AActor ( ActorID InId ) noexcept;
    virtual ~AActor () = default;

public:

    /**
     * @brief Processes one incoming message. Implemented by concrete actors.
     * @param InMessage The message to handle.
     */
    virtual void Receive ( FMessage InMessage ) = 0;

    /**
     * @brief Enqueues a message into this actor's mailbox. Thread-safe.
     * @param InMessage The message to deliver.
     */
    void EnqueueMessage ( FMessage InMessage ) noexcept;

    /** @brief Drains and processes all pending messages. Single-consumer. */
    void ProcessMailbox () noexcept;

public:

    /** @return This actor's unique identifier. */
    [[nodiscard]] ActorID GetId () const noexcept;

private:

    ActorID Id = 0ULL;
    FMailBox Mailbox;

}; // class AActor

} // namespace LumenEngine