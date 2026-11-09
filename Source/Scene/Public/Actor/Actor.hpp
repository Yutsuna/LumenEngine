/**
 * @file Actor.hpp
 * @brief Pure Actor base class for the Lumen Actor Model.
 */

#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorMailbox.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

namespace Lumen
{

/**
 * @class AActor
 * @brief Abstract base for all actors.
 * @details Represents a pure concurrent computational entity.
 */
class SCENE_EXPORT AActor : public FNonCopyable
{
public:

    explicit AActor ( ActorID InId ) noexcept;
    virtual ~AActor () noexcept;

public:

    /** @brief Core logic entry point. Must be implemented by derived actors. */
    virtual void Receive ( const FMessage &InMessage ) noexcept = 0;

    /** @brief Called every tick with the elapsed time since the last tick. */
    virtual void OnTick ( Float64 InDeltaTime ) noexcept;

    /**
     * @brief Lock-free message enqueueing.
     * @return True if the message was successfully enqueued, false otherwise.
     */
    Bool EnqueueMessage ( const FMessage &InMessage ) noexcept;

    /** @brief Freezes the mailbox tick boundary. Called by FWorld::Tick. */
    void BeginTick () noexcept;

    /** @brief Clears the mailbox tick boundary. Called by FWorld::Tick. */
    void EndTick () noexcept;

    /**
     * @brief Runs a single frame of the actor, processing messages and ticking.
     * @param InDeltaTime The elapsed time since the last frame.
     * @param InMaxMessages Maximum number of messages to process in a single call.
     *                      If 0, all pending messages are processed.
     */
    void RunFrame ( Float64 InDeltaTime, UInt32 InMaxMessages = 0U ) noexcept;

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

    /**
     * @brief Single-consumer mailbox processing.
     * @param InMaxMessages Maximum number of messages to process in a single call.
     *                      If 0, all pending messages are processed.
     */
    void ProcessMailbox ( UInt32 InMaxMessages ) noexcept;

private:

    ActorID Id = 0ULL;
    FMailBox Mailbox;
};

} // namespace Lumen
