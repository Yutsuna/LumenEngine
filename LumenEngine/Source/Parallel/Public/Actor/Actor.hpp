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

    /** @brief Single-consumer mailbox processing. */
    void ProcessMailbox () noexcept;

    [[nodiscard]] ActorID GetId () const noexcept;
    [[nodiscard]] FActorRef GetRef () const noexcept;

private:

    ActorID Id = 0ULL;
    FMailBox Mailbox;
};

} // namespace LumenEngine
