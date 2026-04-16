/**
 * @file Actor.hpp
 * @brief Pure Actor base class for the LumenEngine parallel Actor Model.
 */

#pragma once

#include "Actor/ActorMailbox.hpp"
#include "Actor/ActorMessage.hpp"
#include "Actor/ActorTypes.hpp"
#include <typeindex>

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

    /** @brief Returns a unique identifier for the actor class. Used for sorting to optimize I-Cache.
     *  @details Pure C++ implementation using RTTI. Automatically returns the most derived type.
     */
    [[nodiscard]] virtual std::type_index GetTypeIndex () const noexcept;

    /** @brief Lock-free message enqueueing. */
    void EnqueueMessage ( const FMessage &InMessage ) noexcept;

    /**
     * @brief Single-consumer mailbox processing.
     * @param InMaxMessages Maximum number of messages to process in a single call.
     *                      If 0, all pending messages are processed.
     */
    void ProcessMailbox ( UInt32 InMaxMessages = 0U ) noexcept;

    [[nodiscard]] ActorID GetId () const noexcept;
    [[nodiscard]] FActorRef GetRef () const noexcept;

private:

    ActorID Id = 0ULL;
    FMailBox Mailbox;
};

} // namespace LumenEngine
