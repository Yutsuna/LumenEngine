/// @file Actor.hpp
/// @brief Pure Actor base class for Lumen Engine.

#pragma once

#include <SceneExport.hpp>

#include <Core/NonCopyable.hpp>

#include <Actor/ActorMailbox.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

namespace Lumen
{

namespace Internal
{
    class FActorRegistry;
}

///
/// @class AActor
/// @brief Pure Actor base class for Lumen Engine.
///
class SCENE_EXPORT AActor : public FNonCopyable
{
public:

    static constexpr USize DefaultMailboxCapacity   = 1024ULL;
    static constexpr UInt32 DefaultProcessingBudget = 64U;

    /// @param InIndex The index portion of the identity, or zero to allocate one at registration.
    explicit AActor ( UInt32 InIndex            = 0U,
                      USize InMailboxCapacity   = DefaultMailboxCapacity,
                      USize InPreallocatedNodes = 0ULL ) noexcept;
    virtual ~AActor () noexcept;

public:

    /// @brief Publishes a fully constructed actor. An actor cannot be restarted after Shutdown.
    [[nodiscard]] EActorRegistrationResult Initialize () noexcept;

    /// @brief Unpublishes the actor, closes its mailbox, and waits for active sends and Receive calls.
    /// @note Must be called before a derived destructor runs, and must not be called from Receive.
    void Shutdown () noexcept;

    /// @brief Explicit heap-actor destruction entry point. Calls Shutdown while the derived object is still complete.
    /// @note Only for dynamically allocated actors. Must not be called from Receive or more than once.
    void Destroy () noexcept;

    /// @brief Core logic entry point. Implementations must not throw exceptions.
    virtual void Receive ( const FMessage &InMessage ) noexcept = 0;

    [[nodiscard]] EEnqueueResult EnqueueMessage ( const FMessage &InMessage ) noexcept;

    /// @brief Processes at most InMaxMessages messages with exclusive Receive execution.
    /// @param InMaxMessages A finite budget, or zero to explicitly drain until no message is visible.
    /// @return The number of messages processed.
    UInt32 ProcessMailbox ( UInt32 InMaxMessages = DefaultProcessingBudget ) noexcept;

public:

    [[nodiscard]] virtual USize GetTypeID () const noexcept = 0;

    /// @brief Returns an invalid ID before successful registration and the immutable published ID afterwards.
    /// @note Must be externally synchronized with Initialize; safe to call concurrently after publication.
    [[nodiscard]] ActorID GetId () const noexcept;

    /// @brief Returns a reference to the published identity, or an invalid reference before registration.
    /// @note Must be externally synchronized with Initialize; safe to call concurrently after publication.
    [[nodiscard]] FActorRef GetRef () const noexcept;

    /// @brief Returns true if the actor is published and not yet shutdown.
    [[nodiscard]] Bool IsActive () const noexcept;

protected:

    [[nodiscard]] FMailBox &GetMailbox () noexcept;

private:

    static constexpr UInt32 ShutdownFlag   = 1U;
    static constexpr UInt32 ActiveFlag     = 2U;
    static constexpr UInt32 ProcessingFlag = 4U;

    [[nodiscard]] Bool TryActivate () noexcept;
    [[nodiscard]] Bool TryBeginProcessing () noexcept;
    void EndProcessing () noexcept;
    [[nodiscard]] Bool TryAcquireSender () noexcept;
    void ReleaseSender () noexcept;

private:

    friend class Internal::FActorRegistry;

    ActorID Id = {};
    FMailBox Mailbox;
    TAtomic<UInt32> Lifecycle     = { 0U };
    TAtomic<UInt32> ActiveSenders = { 0U };
};

} // namespace Lumen
