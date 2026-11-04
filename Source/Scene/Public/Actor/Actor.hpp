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

class SCENE_EXPORT AActor : public FNonCopyable
{
public:

    explicit AActor ( ActorID InId ) noexcept;
    virtual ~AActor () noexcept;

public:

    /// @brief Core logic entry point. Must be implemented by derived actors.
    virtual void Receive ( const FMessage &InMessage ) = 0;

    /// @brief Lock-free message enqueueing.
    void EnqueueMessage ( const FMessage &InMessage ) noexcept;

    /// @brief Single-consumer mailbox processing.
    /// @param InMaxMessages Maximum number of messages to process in a single call.
    ///                      If 0, all pending messages are processed.
    void ProcessMailbox ( UInt32 InMaxMessages = 0U ) noexcept;

public:

    /// @brief Retrives the unique type ID for this actor instance.
    /// @example
    /// class AMyActor final : public AActor
    /// {
    ///     LUMEN_ACTOR_BODY( MyActor );
    ///
    /// public:
    ///     <Actor Implementation...>
    /// };
    [[nodiscard]] virtual USize GetTypeID () const noexcept = 0;

    /// @brief Retrives the unique ActorID for this actor instance.
    [[nodiscard]] ActorID GetId () const noexcept;

    /// @brief Retrieves a reference to this actor for message sending.
    [[nodiscard]] FActorRef GetRef () const noexcept;

protected:

    [[nodiscard]] FMailBox &GetMailbox () noexcept;

private:

    ActorID Id = 0ULL;
    FMailBox Mailbox;
};

} // namespace Lumen
