/**
 * @file World.hpp
 * @brief Declaration of the FWorld class.
 */

#pragma once

#include <SceneExport.hpp>

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Map.hpp>
#include <Templates/Variant.hpp>
#include <Templates/Vector.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>
#include <Memory/UniquePtr.hpp>

#include <Actor/ActorEmitContext.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>
#include <World/WorldSpatialRegistry.hpp>
#include <World/WorldTypes.hpp>

namespace Lumen
{

class FWorldExecutor;

/**
 * @class FWorld
 * @brief Owns actors and executes explicit phases separated by CPU fences.
 */
class SCENE_EXPORT FWorld : public FNonCopyable
{
public:

    FWorld () noexcept;
    explicit FWorld ( const FWorldDesc &InDesc ) noexcept;
    virtual ~FWorld () noexcept;

public:

    /** @brief Legacy infallible spawn adapter; new code should consume TrySpawnActor. */
    template <typename ActorType, typename... Args> [[nodiscard]] TSharedRef<ActorType> SpawnActor ( Args &&...InArgs );

    /** @brief Spawns and activates an actor without terminating on registry or capacity exhaustion. */
    template <typename ActorType, typename... Args>
    [[nodiscard]] TExpected<TSharedRef<ActorType>, EActorSpawnError> TrySpawnActor ( Args &&...InArgs ) noexcept;

    /** @brief Legacy deferred spawn adapter; new code should consume TrySpawnActorDeferred. */
    template <typename ActorType, typename... Args> [[nodiscard]] FActorRef SpawnActorDeferred ( Args &&...InArgs );

    /**
     * @brief Constructs during this control operation, then activates at the next serial commit.
     * @details The ID is reserved immediately but routing and BeginPlay stay closed until activation.
     */
    template <typename ActorType, typename... Args>
    [[nodiscard]] TExpected<FActorRef, EActorSpawnError> TrySpawnActorDeferred ( Args &&...InArgs ) noexcept;

    /** @brief Legacy destroy adapter. */
    void DestroyActor ( ActorID InId ) noexcept;

    /** @brief Requests logical destruction, deferring it when called from an actor task. */
    [[nodiscard]] TExpected<void, EActorDestroyError> TryDestroyActor ( ActorID InId ) noexcept;

    /** @brief Legacy broadcast adapter. */
    void Broadcast ( const FMessage &InMessage ) noexcept;

    /** @brief Broadcasts now or through one bounded canonical deferred command. */
    [[nodiscard]] TExpected<void, EActorSendError> TryBroadcast ( const FMessage &InMessage ) noexcept;

    /** @brief Legacy tick adapter that discards the structured result. */
    void Tick ( Float64 InDeltaTime ) noexcept;

    /** @brief Runs fixed steps, PreUpdate, Update, PostUpdate, then merges actor outboxes. */
    [[nodiscard]] TExpected<FWorldTickResult, EWorldTickError> TryTick ( Float64 InDeltaTime ) noexcept;

    /** @brief Runs exactly one explicit fixed step, independently from the variable accumulator. */
    [[nodiscard]] TExpected<FWorldTickResult, EWorldTickError> TryFixedTick ( Float64 InDeltaTime ) noexcept;

    /** @returns True while an actor phase is active on the executor. */
    [[nodiscard]] Bool IsInParallelPhase () const noexcept;

    /** @brief Returns the most recently completed phase token. */
    [[nodiscard]] FWorldPhaseCompletion GetLastPhaseCompletion () const noexcept;

    /** @brief Returns this world's owned spatial registry. */
    [[nodiscard]] FSpatialRegistry &GetSpatialRegistry () noexcept;

public:

    static constexpr UInt32 MaxMessagesPerTick = 100U;

private:

    struct FDeferredSpawnCommand
    {
        TSharedPtr<AActor> Actor;
    };

    struct FDeferredDestroyCommand
    {
        ActorID Target = InvalidActorID;
    };

    struct FDeferredBroadcastCommand
    {
        FMessage Message;
    };

    using FDeferredCommandPayload = TVariant<FDeferredSpawnCommand, FDeferredDestroyCommand, FDeferredBroadcastCommand>;

    struct FDeferredCommand
    {
        ActorID Sender        = InvalidActorID;
        UInt64 SenderSequence = 0ULL;
        ActorID Target        = InvalidActorID;
        FDeferredCommandPayload Payload;
    };

    [[nodiscard]] TExpected<void, EActorSpawnError> InsertSpawnedActor ( const TSharedPtr<AActor> &InActor ) noexcept;
    [[nodiscard]] Bool QueueDeferredCommand ( ActorID InTarget, FDeferredCommandPayload &&InPayload ) noexcept;
    [[nodiscard]] TExpected<void, EActorDestroyError> DestroyActorImpl ( ActorID InId ) noexcept;
    [[nodiscard]] TExpected<FWorldPhaseCompletion, EWorldTickError>
    RunPhase ( TSpan<AActor *const> InActors, EActorTickPhase InPhase, UInt64 InFixedStepIndex, Float64 InDeltaTime ) noexcept;
    [[nodiscard]] TExpected<void, EWorldTickError>
    RunFixedStep ( UInt64 InFixedStepIndex, Float64 InDeltaTime, FWorldTickResult &OutResult ) noexcept;
    [[nodiscard]] Bool MergeEmissions ( TSpan<AActor *const> InActors, FWorldTickResult &OutResult ) noexcept;

    Bool ApplyDeferredCommands () noexcept;
    void SortActorsIfNeeded () noexcept;
    void ShutdownActors () noexcept;

private:

    FWorldDesc Desc;
    FSpatialRegistry Registry;
    TUniquePtr<FWorldExecutor> Executor;

    TMap<ActorID, TSharedPtr<AActor>> Actors;
    TVector<TSharedPtr<AActor>> ActiveActors;

    mutable HAL::FPlatformMutex DeferredMutex;
    TVector<FDeferredCommand> DeferredCommands;
    TVector<FDeferredCommand> DeferredCommandBatch;
    UInt64 NextExternalCommandSequence = 0ULL;

    TVector<AActor *> PhaseActors;
    TVector<AActor *> VariableActors;
    TVector<FActorEmission> MergedEmissions;

    FWorldPhaseCompletion LastPhase;
    Float64 FixedAccumulator = 0.0;
    UInt64 FrameIndex        = 0ULL;
    Bool bNeedsSorting       = false;
    Bool bInParallelPhase    = false;
    Bool bShuttingDown       = false;
};

namespace Internal
{

    /** @brief Legacy adapters retain their historical fatal failure contract. */
    [[noreturn]] inline void TerminateOnSpawnFailure ( EActorSpawnError InError ) noexcept;

} // namespace Internal

} // namespace Lumen

#include "Inline/World.inl"
