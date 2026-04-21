/**
 * @file Actor.hpp
 * @brief Pure Actor base class for the LumenEngine parallel Actor Model.
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
 * @details Represents a pure concurrent computational entity.
 */
class LUMEN_ENGINE_API AActor : public FNonCopyable
{
public:

    explicit AActor ( ActorID InId ) noexcept;
    virtual ~AActor () noexcept;

public:

    /** @brief Core logic entry point. Must be implemented by derived actors. */
    virtual void Receive ( const FMessage &InMessage ) = 0;

    /** @brief Lock-free message enqueueing. */
    void EnqueueMessage ( const FMessage &InMessage ) noexcept;

    /**
     * @brief Single-consumer mailbox processing.
     * @param InMaxMessages Maximum number of messages to process in a single call.
     *                      If 0, all pending messages are processed.
     */
    void ProcessMailbox ( UInt32 InMaxMessages = 0U ) noexcept;

    /**
     * @brief Retrieves the unique type ID for this actor instance.
     * @return A unique identifier representing the actor's type.
     *
     * @example:
     *  class MyActor final : public AActor
     *  {
     *       LUMEN_ACTOR_BODY( MyActor );
     *
     *   public:
     *       // Actor implementation...
     *  };
     */
    [[nodiscard]] virtual USize GetTypeID () const noexcept = 0;

    /**
     * @brief Retrieves the unique ActorID for this actor instance.
     * @return The unique ActorID assigned to this actor.
     */
    [[nodiscard]] ActorID GetId () const noexcept;

    /**
     * @brief Retrieves a reference to this actor for message sending.
     * @return An FActorRef that can be used to send messages to this actor.
     */
    [[nodiscard]] FActorRef GetRef () const noexcept;

protected:

    /** @brief Provides access to the mailbox for subclasses (e.g. for Reserve()). */
    [[nodiscard]] FMailBox &GetMailbox () noexcept; 

private:

    ActorID Id = 0ULL;
    FMailBox Mailbox;
};

} // namespace LumenEngine
