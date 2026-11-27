/**
 * @file Actor.hpp
 * @brief Pure Actor base class for the Lumen Actor Model.
 */

#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorEmitContext.hpp>
#include <Actor/ActorMailbox.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

namespace Lumen
{

struct FWorldFrameContext;
class FWorldExecutor;

/**
 * @class AActor
 * @brief Abstract base for all actors with an explicit logical lifetime.
 */
class SCENE_EXPORT AActor : public FNonCopyable
{
public:

    explicit AActor ( ActorID InId ) noexcept;
    virtual ~AActor () noexcept;

public:

    /** @brief Core logic entry point. Must be implemented by derived actors. */
    virtual void Receive ( const FMessage &InMessage ) noexcept = 0;

    /** @brief Called once after the fully constructed actor joins a world or runs standalone. */
    virtual void OnBeginPlay () noexcept;

    /** @brief Called once when logical retirement starts, before C++ storage is released. */
    virtual void OnEndPlay () noexcept;

    /** @brief Called every non-fixed tick with the elapsed time since the last tick. */
    virtual void OnTick ( Float64 InDeltaTime ) noexcept;

    /** @brief Called once per fixed simulation step for FixedUpdate actors. */
    virtual void OnFixedTick ( Float64 InDeltaTime ) noexcept;

    /** @brief Selects the single explicit world phase executed by this actor. */
    [[nodiscard]] virtual EActorTickPhase GetTickPhase () const noexcept;

    /** @brief Enqueues through this actor's mailbox, unless logically retired. */
    Bool EnqueueMessage ( const FMessage &InMessage ) noexcept;

    /** @brief Emits through the world outbox during a task, or uses legacy routing outside one. */
    [[nodiscard]] TExpected<void, EActorSendError> TrySendMessage ( ActorID InTarget, const FMessage &InMessage ) noexcept;

    /** @brief Freezes the mailbox tick boundary. Called by FWorld::Tick. */
    void BeginTick () noexcept;

    /** @brief Clears the mailbox tick boundary. Called by FWorld::Tick. */
    void EndTick () noexcept;

    /** @brief Standalone compatibility adapter processing mail then executing the selected tick. */
    void RunFrame ( Float64 InDeltaTime, UInt32 InMaxMessages = 0U ) noexcept;

    /** @brief Retrieves the unique type ID for this actor instance. */
    [[nodiscard]] virtual USize GetTypeID () const noexcept = 0;

    /** @brief Retrieves the generation-aware actor ID. */
    [[nodiscard]] ActorID GetId () const noexcept;

    /** @brief Retrieves a non-owning communication handle. */
    [[nodiscard]] FActorRef GetRef () const noexcept;

    /** @brief Returns the logical state, independently from external shared references. */
    [[nodiscard]] EActorLifecycle GetLifecycle () const noexcept;

protected:

    /** @brief Provides access to the mailbox for subclasses (e.g. for Reserve()). */
    [[nodiscard]] FMailBox &GetMailbox () noexcept;

private:

    void BeginPlay () noexcept;
    void SuspendRouting () noexcept;
    void MarkPendingKill () noexcept;
    void EndPlay () noexcept;
    void Retire () noexcept;
    void ReserveOutbox ( USize InCapacity );
    void RunWorldFrame ( const FWorldFrameContext &InContext, Float64 InDeltaTime, UInt32 InMaxMessages ) noexcept;
    void ProcessMailbox ( UInt32 InMaxMessages ) noexcept;

    [[nodiscard]] static FActorEmitContext *GetCurrentEmitContext () noexcept;

private:

    ActorID Id = 0ULL;
    FMailBox Mailbox;
    FActorEmitContext EmitContext;
    TAtomic<EActorLifecycle> Lifecycle = { EActorLifecycle::Spawning };
    Bool bRegistered                   = false;
    Bool bBegunPlay                    = false;
    Bool bEndedPlay                    = false;

    friend class FWorld;
    friend class FWorldExecutor;
};

} // namespace Lumen
