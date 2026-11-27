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
#include <Templates/Optional.hpp>
#include <Templates/Variant.hpp>
#include <Templates/Vector.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>
#include <Memory/UniquePtr.hpp>

#include <Actor/ActorEmitContext.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>
#include <Hierarchy/SceneHierarchy.hpp>
#include <Rendering/RenderSnapshotStream.hpp>
#include <Rendering/RenderSnapshotTypes.hpp>
#include <Rendering/RenderSnapshotView.hpp>
#include <Rendering/SceneExtractor.hpp>
#include <World/WorldIngressJournal.hpp>
#include <World/WorldRenderContent.hpp>
#include <World/WorldSpatialRegistry.hpp>
#include <World/WorldTypes.hpp>

namespace Lumen
{

class FWorldExecutor;
class FReplicationSystem;
class FWorldPartition;
class IReplicationTransport;

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

    /**
     * @brief Installs an external ingress journal consumed serially at each tick.
     * @details The world does not own the journal. Installing one resets the replay cursor.
     */
    void SetIngressJournal ( const FWorldIngressJournal *InJournal ) noexcept;

    /** @brief Returns the number of ingress records applied by the most recent tick. */
    [[nodiscard]] UInt64 GetLastIngressRecordCount () const noexcept;

    /** @brief Returns the frame index that the next tick will produce. */
    [[nodiscard]] UInt64 GetFrameIndex () const noexcept;

    /** @returns True while an actor phase is active on the executor. */
    [[nodiscard]] Bool IsInParallelPhase () const noexcept;

    /** @brief Returns the most recently completed phase token. */
    [[nodiscard]] FWorldPhaseCompletion GetLastPhaseCompletion () const noexcept;

    /** @brief Returns this world's owned spatial registry. */
    [[nodiscard]] FSpatialRegistry &GetSpatialRegistry () noexcept;

    /** @brief Returns this world's owned spatial hierarchy; Initialize depends on FWorldDesc::HierarchyCapacity. */
    [[nodiscard]] FSceneHierarchy &GetHierarchy () noexcept;

    /** @brief Opt-in registration of an existing actor as a hierarchy node. */
    [[nodiscard]] TExpected<void, ESceneHierarchyError>
    TryRegisterHierarchyActor ( ActorID InActorId,
                                const Maths::FMat4x4f &InLocalTransform = Maths::FMat4x4f::Identity() ) noexcept;

    /** @brief Attaches a node to a weak parent ID; deferred through the canonical command queue inside a phase. */
    [[nodiscard]] TExpected<void, ESceneHierarchyError> TryHierarchyAttach ( ActorID InActorId, ActorID InParentId ) noexcept;

    /** @brief Detaches a node, preserving its world transform as its new local transform. */
    [[nodiscard]] TExpected<void, ESceneHierarchyError> TryHierarchyDetach ( ActorID InActorId ) noexcept;

    /** @brief Changes a node's local transform, resolving descendants at the next serial hierarchy pass. */
    [[nodiscard]] TExpected<void, ESceneHierarchyError>
    TrySetHierarchyLocalTransform ( ActorID InActorId, const Maths::FMat4x4f &InLocalTransform ) noexcept;

    /**
     * @brief Injects the replication collaborators and reserves replication storage.
     * @details Uses FWorldDesc::ReplicationConfig/ReplicationRole. A zero MaxActors is rejected;
     *          the world never creates a transport or an actor handler itself.
     */
    [[nodiscard]] TExpected<void, EReplicationError> InitializeReplication ( IReplicationTransport &InTransport,
                                                                             IReplicationActorHandler &InHandler ) noexcept;

    /** @return The world-owned replication system, or nullptr when replication was not installed. */
    [[nodiscard]] FReplicationSystem *GetReplicationSystem () const noexcept;

    /** @return True when a replication system is installed and initialized. */
    [[nodiscard]] Bool IsReplicationEnabled () const noexcept;

    /** @return The world-owned partition, or nullptr when no valid configuration was supplied. */
    [[nodiscard]] FWorldPartition *GetPartition () const noexcept;

    /** @return True when a partition was successfully reserved at construction. */
    [[nodiscard]] Bool IsPartitionEnabled () const noexcept;

    /**
     * @brief Assigns an existing actor to a cell; control-time only.
     * @details When replication is active the persistent label is reconciled with the authority/replica mapping;
     *          M10 never allocates a persistent identity itself.
     */
    [[nodiscard]] TExpected<void, EWorldPartitionError>
    TryAssignActorToCell ( ActorID InActorId, FWorldCellId InCellId, FPersistentActorId InPersistentId ) noexcept;

    /** @brief Removes an actor from its cell; control-time only. */
    [[nodiscard]] TExpected<void, EWorldPartitionError> TryUnassignActorFromCell ( ActorID InActorId ) noexcept;

    /** @brief Requests a cell load, deferred through the canonical queue during an actor phase. */
    [[nodiscard]] TExpected<void, EWorldPartitionError> TryPartitionLoadCell ( FWorldCellId InCellId ) noexcept;

    /** @brief Requests a cell unload, deferred through the canonical queue during an actor phase. */
    [[nodiscard]] TExpected<void, EWorldPartitionError> TryPartitionUnloadCell ( FWorldCellId InCellId ) noexcept;

    /** @return True when the actor belongs to a cell whose state is Unloaded. */
    [[nodiscard]] Bool IsActorDormant ( ActorID InActorId ) const noexcept;

    /** @return The stable non-zero identity carried by every published snapshot. */
    [[nodiscard]] UInt64 GetWorldId () const noexcept;

    /** @return True when at least one render capacity was configured, enabling automatic extraction. */
    [[nodiscard]] Bool IsRenderExtractionEnabled () const noexcept;

    /** @brief Returns the world-owned latest-only rendering snapshot stream. */
    [[nodiscard]] FRenderSnapshotStream &GetRenderSnapshotStream () noexcept;

    /**
     * @brief Publishes one complete snapshot from committed state and authored content.
     * @details Called automatically after the last world fence when extraction is enabled. Control-time callers
     *          may invoke it directly to force a publication without advancing the simulation. A capacity or
     *          source-index failure leaves the previous published snapshot intact.
     */
    [[nodiscard]] TExpected<FRenderSnapshotHeader, FRenderSnapshotError> PublishRenderSnapshot () noexcept;

    /** @brief Promotes the newest pending snapshot on the single render-reader thread. */
    Bool SwapRenderSnapshotReadBuffers () noexcept;

    /** @return The immutable view currently exposed to the render reader. */
    [[nodiscard]] FRenderSnapshotView GetRenderSnapshot () const noexcept;

    /** @return The last rejected publication, if any; a later success clears it. */
    [[nodiscard]] TOptional<FRenderSnapshotError> GetLastRenderError () const noexcept;

    /** @return Number of successful publications performed by this world. */
    [[nodiscard]] UInt64 GetRenderPublishCount () const noexcept;

    /**
     * @brief Returns control-time authored mesh/camera/light records for future publications.
     * @note Mutate only outside actor phases; transforms are always derived from committed spatial state.
     */
    [[nodiscard]] FWorldRenderContent &GetRenderContent () noexcept;

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

    struct FDeferredHierarchyRegisterCommand
    {
        ActorID ActorId       = InvalidActorID;
        Maths::FMat4x4f Local = Maths::FMat4x4f::Identity();
    };

    struct FDeferredHierarchyAttachCommand
    {
        ActorID ActorId  = InvalidActorID;
        ActorID ParentId = InvalidActorID;
    };

    struct FDeferredHierarchyDetachCommand
    {
        ActorID ActorId = InvalidActorID;
    };

    struct FDeferredHierarchyLocalTransformCommand
    {
        ActorID ActorId       = InvalidActorID;
        Maths::FMat4x4f Local = Maths::FMat4x4f::Identity();
    };

    struct FDeferredPartitionLoadCommand
    {
        FWorldCellId CellId;
    };

    struct FDeferredPartitionUnloadCommand
    {
        FWorldCellId CellId;
    };

    /** INFO: New alternatives are appended so existing Payload.index() values and canonical ordering stay stable. */
    using FDeferredCommandPayload = TVariant<FDeferredSpawnCommand,
                                             FDeferredDestroyCommand,
                                             FDeferredBroadcastCommand,
                                             FDeferredHierarchyRegisterCommand,
                                             FDeferredHierarchyAttachCommand,
                                             FDeferredHierarchyDetachCommand,
                                             FDeferredHierarchyLocalTransformCommand,
                                             FDeferredPartitionLoadCommand,
                                             FDeferredPartitionUnloadCommand>;

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
    [[nodiscard]] TExpected<void, ESceneHierarchyError>
    ApplyHierarchyRegister ( ActorID InActorId, const Maths::FMat4x4f &InLocalTransform ) noexcept;
    [[nodiscard]] TExpected<void, ESceneHierarchyError> ApplyHierarchyAttach ( ActorID InActorId, ActorID InParentId ) noexcept;
    [[nodiscard]] TExpected<void, ESceneHierarchyError> ApplyHierarchyDetach ( ActorID InActorId ) noexcept;
    [[nodiscard]] TExpected<void, ESceneHierarchyError>
    ApplyHierarchyLocalTransform ( ActorID InActorId, const Maths::FMat4x4f &InLocalTransform ) noexcept;
    void ApplyHierarchyWorldTransforms () noexcept;
    [[nodiscard]] TExpected<void, EWorldPartitionError> ApplyPartitionLoadCell ( FWorldCellId InCellId,
                                                                                 UInt64 InSenderSequence ) noexcept;
    [[nodiscard]] TExpected<void, EWorldPartitionError> ApplyPartitionUnloadCell ( FWorldCellId InCellId,
                                                                                   UInt64 InSenderSequence ) noexcept;
    void DetachExternalChildrenOfCell ( FWorldCellId InCellId ) noexcept;
    void SetCellActorRouting ( FWorldCellId InCellId, Bool bInRoutable ) noexcept;
    [[nodiscard]] Bool IsRenderProxyDormant ( FRenderProxyId InProxyId ) const noexcept;
    [[nodiscard]] TExpected<FWorldPhaseCompletion, EWorldTickError>
    RunPhase ( TSpan<AActor *const> InActors, EActorTickPhase InPhase, UInt64 InFixedStepIndex, Float64 InDeltaTime ) noexcept;
    [[nodiscard]] TExpected<void, EWorldTickError>
    RunFixedStep ( UInt64 InFixedStepIndex, Float64 InDeltaTime, FWorldTickResult &OutResult ) noexcept;
    [[nodiscard]] Bool MergeEmissions ( TSpan<AActor *const> InActors, FWorldTickResult &OutResult ) noexcept;

    void ApplyIngressJournal () noexcept;
    void TickReplication () noexcept;
    Bool ApplyDeferredCommands () noexcept;
    void SortActorsIfNeeded () noexcept;
    void ShutdownActors () noexcept;

private:

    FWorldDesc Desc;
    FSpatialRegistry Registry;
    FSceneHierarchy Hierarchy;
    TUniquePtr<FWorldExecutor> Executor;
    TUniquePtr<FReplicationSystem> Replication;
    TVector<FReplicationActorSourceRecord> ReplicationSourceScratch;
    TUniquePtr<FWorldPartition> Partition;

    /** INFO: The extraction stream lives per world; no actor, asset or renderer ownership crosses this boundary. */
    FRenderSnapshotStream RenderSnapshotStream;
    FSceneExtractor RenderExtractor;
    FWorldRenderContent RenderContent;
    TVector<FRenderTransformRecord> RenderTransforms;
    TVector<FRenderMeshRecord> RenderMeshes;
    TVector<FRenderCameraRecord> RenderCameras;
    TVector<FRenderLightRecord> RenderLights;
    FRenderSnapshotError LastRenderError;
    UInt64 WorldId            = 0ULL;
    UInt64 RenderPublishCount = 0ULL;
    Bool bHasRenderError      = false;

    TMap<ActorID, TSharedPtr<AActor>> Actors;
    TVector<TSharedPtr<AActor>> ActiveActors;

    mutable HAL::FPlatformMutex DeferredMutex;
    TVector<FDeferredCommand> DeferredCommands;
    TVector<FDeferredCommand> DeferredCommandBatch;
    UInt64 NextExternalCommandSequence = 0ULL;

    TVector<AActor *> PhaseActors;
    TVector<AActor *> VariableActors;
    TVector<ActorID> PartitionScratch;
    TVector<FActorEmission> MergedEmissions;
    TVector<FWorldIngressRecord> IngressBatch;

    const FWorldIngressJournal *IngressJournal = nullptr;
    USize IngressCursor                        = 0ULL;
    UInt64 IngressGeneration                   = 0ULL;
    UInt64 LastIngressRecordCount              = 0ULL;

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
