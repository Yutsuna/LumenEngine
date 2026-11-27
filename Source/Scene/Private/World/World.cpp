/**
 * @file World.cpp
 * @brief Implementation of the FWorld class.
 */

#include <World/World.hpp>
#include <World/WorldExecutor.hpp>
#include <World/WorldSpatialRegistry.hpp>

#include <Replication/ReplicationSystem.hpp>
#include <WorldPartition/WorldPartition.hpp>

#include <Algo/ForEach.hpp>
#include <Algo/Sort.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorRegistry.hpp>

#include <Meta/Overloaded.hpp>
#include <Templates/Optional.hpp>

namespace
{

/** @brief Process-wide fallback identities; explicit FWorldDesc::WorldId values remain untouched. */
Lumen::TAtomic<Lumen::UInt64> GNextWorldId{ 1ULL };

[[nodiscard]] Lumen::FRenderSnapshotError MakeCapacityError ( const Lumen::ERenderSnapshotRecordType InRecordType,
                                                              const Lumen::USize InRequiredCount,
                                                              const Lumen::USize InCapacity ) noexcept
{
    return Lumen::FRenderSnapshotError{
        .Code          = Lumen::ERenderSnapshotErrorCode::CapacityExceeded,
        .RecordType    = InRecordType,
        .RequiredCount = InRequiredCount,
        .Capacity      = InCapacity,
    };
}

/**
 * @brief Rank of a proxy in the already canonical transform array.
 * @return The rank, or the array size when the proxy has no committed transform.
 */
[[nodiscard]] Lumen::USize FindProxyRank ( const Lumen::TVector<Lumen::FRenderTransformRecord> &InTransforms,
                                           const Lumen::FRenderProxyId InProxyId ) noexcept
{
    Lumen::USize Lower = 0ULL;
    Lumen::USize Upper = InTransforms.size();

    while ( Lower < Upper )
    {
        const Lumen::USize Middle = Lower + ( ( Upper - Lower ) / 2ULL );
        if ( InTransforms[Middle].ProxyId < InProxyId )
        {
            Lower = Middle + 1ULL;
        }
        else
        {
            Upper = Middle;
        }
    }

    return Lower < InTransforms.size() and InTransforms[Lower].ProxyId == InProxyId ? Lower : InTransforms.size();
}

[[nodiscard]] Lumen::Bool SortRenderTransforms ( const Lumen::FRenderTransformRecord &InLeft,
                                                 const Lumen::FRenderTransformRecord &InRight ) noexcept
{
    return InLeft.ProxyId < InRight.ProxyId;
}

[[nodiscard]] Lumen::Bool SortActiveActors ( const Lumen::TSharedPtr<Lumen::AActor> &InLeft,
                                             const Lumen::TSharedPtr<Lumen::AActor> &InRight ) noexcept
{
    const Lumen::USize LeftType  = InLeft->GetTypeID();
    const Lumen::USize RightType = InRight->GetTypeID();
    return LeftType != RightType ? LeftType < RightType : InLeft->GetId() < InRight->GetId();
}

[[nodiscard]] Lumen::Bool SortEmissions ( const Lumen::FActorEmission &InLeft, const Lumen::FActorEmission &InRight ) noexcept
{
    if ( InLeft.Target != InRight.Target )
    {
        return InLeft.Target < InRight.Target;
    }
    if ( InLeft.Sender != InRight.Sender )
    {
        return InLeft.Sender < InRight.Sender;
    }
    return InLeft.SenderSequence < InRight.SenderSequence;
}

} // namespace

Lumen::FWorld::FWorld () noexcept : FWorld( FWorldDesc{} )
{
}

Lumen::FWorld::FWorld ( const FWorldDesc &InDesc ) noexcept
    : Desc( InDesc ), Registry( FSpatialRegistryCapacities{ .EntityCapacity = InDesc.MaxActors } ),
      Executor( MakeUnique<FWorldExecutor>( InDesc.WorkerCount, InDesc.MaxActors ) ),
      RenderSnapshotStream( InDesc.RenderCapacities ), RenderExtractor( RenderSnapshotStream ),
      WorldId( InDesc.WorldId != 0ULL ? InDesc.WorldId : GNextWorldId.fetch_add( 1ULL, std::memory_order_relaxed ) )
{
    Actors.reserve( Desc.MaxActors );
    ActiveActors.reserve( Desc.MaxActors );
    DeferredCommands.reserve( Desc.DeferredCommandCapacity );
    DeferredCommandBatch.reserve( Desc.DeferredCommandCapacity );
    PhaseActors.reserve( Desc.MaxActors );
    VariableActors.reserve( Desc.MaxActors );
    MergedEmissions.reserve( Desc.EmissionCapacity );
    IngressBatch.reserve( FWorldIngressJournal::DefaultCapacity );

    /** INFO: Publication itself must never allocate; all record arrays are reserved once at construction. */
    RenderTransforms.reserve( Desc.RenderCapacities.TransformCapacity );
    RenderMeshes.reserve( Desc.RenderCapacities.MeshCapacity );
    RenderContent.Meshes.reserve( Desc.RenderCapacities.MeshCapacity );
    RenderContent.Cameras.reserve( Desc.RenderCapacities.CameraCapacity );
    RenderContent.Lights.reserve( Desc.RenderCapacities.LightCapacity );
    RenderCameras.reserve( Desc.RenderCapacities.CameraCapacity );
    RenderLights.reserve( Desc.RenderCapacities.LightCapacity );
    ReplicationSourceScratch.reserve( Desc.MaxActors );
    PartitionScratch.reserve( Desc.MaxActors > Desc.HierarchyCapacity ? Desc.MaxActors : Desc.HierarchyCapacity );

    /** INFO: Hierarchy is opt-in; a failed reservation leaves it disabled and never terminates the world. */
    if ( Desc.HierarchyCapacity > 0ULL )
    {
        static_cast<void>( Hierarchy.Initialize( Desc.HierarchyCapacity ) );
    }

    /** INFO: Partitioning is opt-in; a failed or partial reservation leaves the world unpartitioned. */
    if ( Desc.PartitionConfig.MaxCells > 0ULL )
    {
        TUniquePtr<FWorldPartition> Reserved = MakeUnique<FWorldPartition>();
        if ( Reserved->Initialize( Desc.PartitionConfig ) )
        {
            Partition = std::move( Reserved );
        }
    }
}

Lumen::FWorld::~FWorld () noexcept
{
    bShuttingDown = true;
    Executor.Reset();
    ShutdownActors();
}

void Lumen::FWorld::Broadcast ( const FMessage &InMessage ) noexcept
{
    static_cast<void>( TryBroadcast( InMessage ) );
}

Lumen::TExpected<void, Lumen::EActorSendError> Lumen::FWorld::TryBroadcast ( const FMessage &InMessage ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EActorSendError::WorldClosed );
    }

    if ( bInParallelPhase )
    {
        if ( not QueueDeferredCommand( InvalidActorID, FDeferredBroadcastCommand{ .Message = InMessage } ) )
        {
            return MakeUnexpected( EActorSendError::DeferredQueueFull );
        }
        return {};
    }

    TOptional<EActorSendError> FirstError;
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        if ( Actor->GetLifecycle() != EActorLifecycle::Active or IsActorDormant( Actor->GetId() ) )
        {
            continue;
        }

        TExpected<void, EActorSendError> Result =
            Internal::FActorRegistry::TryEnqueueMessage( Actor->GetId(), InMessage, FrameIndex );
        if ( not Result and not FirstError )
        {
            FirstError.Emplace( Result.Error() );
        }
    }

    if ( FirstError )
    {
        return MakeUnexpected( *FirstError );
    }
    return {};
}

void Lumen::FWorld::DestroyActor ( const ActorID InId ) noexcept
{
    static_cast<void>( TryDestroyActor( InId ) );
}

Lumen::TExpected<void, Lumen::EActorDestroyError> Lumen::FWorld::TryDestroyActor ( const ActorID InId ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EActorDestroyError::WorldClosed );
    }
    if ( InId == InvalidActorID )
    {
        return MakeUnexpected( EActorDestroyError::InvalidActor );
    }

    if ( bInParallelPhase )
    {
        if ( not QueueDeferredCommand( InId, FDeferredDestroyCommand{ .Target = InId } ) )
        {
            return MakeUnexpected( EActorDestroyError::DeferredQueueFull );
        }
        return {};
    }

    return DestroyActorImpl( InId );
}

void Lumen::FWorld::Tick ( const Float64 InDeltaTime ) noexcept
{
    static_cast<void>( TryTick( InDeltaTime ) );
}

Lumen::TExpected<Lumen::FWorldTickResult, Lumen::EWorldTickError> Lumen::FWorld::TryTick ( const Float64 InDeltaTime ) noexcept
{
    if ( bShuttingDown or not Executor.IsValid() )
    {
        return MakeUnexpected( EWorldTickError::ExecutorUnavailable );
    }

    ++FrameIndex;
    FWorldTickResult Result{ .FrameIndex = FrameIndex, .LastPhase = {} };
    TOptional<EWorldTickError> FirstError;

    if ( ApplyDeferredCommands() )
    {
        FirstError.Emplace( EWorldTickError::WorldCapacityExceeded );
    }
    ApplyIngressJournal();
    SortActorsIfNeeded();
    TickReplication();

    VariableActors.clear();
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        if ( Actor->GetLifecycle() == EActorLifecycle::Active and Actor->GetTickPhase() != EActorTickPhase::FixedUpdate and
             not IsActorDormant( Actor->GetId() ) )
        {
            VariableActors.push_back( Actor.Get() );
            Actor->BeginTick();
        }
    }

    const Float64 DeltaTime = InDeltaTime > 0.0 ? InDeltaTime : 0.0;
    if ( Desc.FixedDeltaTime > 0.0 )
    {
        FixedAccumulator += DeltaTime;
        const Float64 PendingSteps = FixedAccumulator / Desc.FixedDeltaTime;
        UInt32 StepCount           = 0U;

        if ( PendingSteps > static_cast<Float64>( Desc.MaxFixedStepsPerTick ) )
        {
            StepCount        = Desc.MaxFixedStepsPerTick;
            FixedAccumulator = 0.0;
            if ( not FirstError )
            {
                FirstError.Emplace( EWorldTickError::FixedStepLimitExceeded );
            }
        }
        else
        {
            StepCount = static_cast<UInt32>( PendingSteps );
            FixedAccumulator -= static_cast<Float64>( StepCount ) * Desc.FixedDeltaTime;
        }

        for ( UInt32 StepIndex = 0U; StepIndex < StepCount; ++StepIndex )
        {
            TExpected<void, EWorldTickError> StepResult = RunFixedStep( StepIndex, Desc.FixedDeltaTime, Result );
            if ( not StepResult )
            {
                for ( AActor *Actor : VariableActors )
                {
                    Actor->EndTick();
                }
                return MakeUnexpected( StepResult.Error() );
            }
            ++Result.FixedStepsExecuted;
        }
    }

    constexpr EActorTickPhase VariablePhases[] = {
        EActorTickPhase::PreUpdate,
        EActorTickPhase::Update,
        EActorTickPhase::PostUpdate,
    };

    for ( const EActorTickPhase Phase : VariablePhases )
    {
        PhaseActors.clear();
        for ( AActor *Actor : VariableActors )
        {
            if ( Actor->GetTickPhase() == Phase )
            {
                PhaseActors.push_back( Actor );
            }
        }

        TExpected<FWorldPhaseCompletion, EWorldTickError> Completion = RunPhase(
            TSpan<AActor *const>( PhaseActors.data(), PhaseActors.size() ), Phase, Result.FixedStepsExecuted, DeltaTime );
        if ( not Completion )
        {
            for ( AActor *Actor : VariableActors )
            {
                Actor->EndTick();
            }
            return MakeUnexpected( Completion.Error() );
        }

        Result.DispatchedTaskCount += Completion->CompletedTaskCount;
        LastPhase = *Completion;
    }

    for ( AActor *Actor : VariableActors )
    {
        Actor->EndTick();
    }

    if ( not MergeEmissions( TSpan<AActor *const>( VariableActors.data(), VariableActors.size() ), Result ) and not FirstError )
    {
        FirstError.Emplace( EWorldTickError::EmissionCapacityExceeded );
    }

    /** INFO: Single serial hierarchy pass after the last phase fence, before spatial publication. */
    if ( Hierarchy.IsInitialized() and Hierarchy.UpdateWorld() )
    {
        ApplyHierarchyWorldTransforms();
    }

    Registry.Publish();

    if ( IsRenderExtractionEnabled() )
    {
        /** INFO: Render publication is observable but never masks the simulation result of this tick. */
        static_cast<void>( PublishRenderSnapshot() );
    }

    if ( FirstError )
    {
        return MakeUnexpected( *FirstError );
    }
    return Result;
}

Lumen::TExpected<Lumen::FWorldTickResult, Lumen::EWorldTickError>
Lumen::FWorld::TryFixedTick ( const Float64 InDeltaTime ) noexcept
{
    if ( bShuttingDown or not Executor.IsValid() )
    {
        return MakeUnexpected( EWorldTickError::ExecutorUnavailable );
    }

    ++FrameIndex;
    ApplyDeferredCommands();
    ApplyIngressJournal();
    SortActorsIfNeeded();
    TickReplication();

    FWorldTickResult Result{ .FrameIndex = FrameIndex, .FixedStepsExecuted = 1U, .LastPhase = {} };
    TExpected<void, EWorldTickError> StepResult = RunFixedStep( 0ULL, InDeltaTime, Result );
    if ( not StepResult )
    {
        return MakeUnexpected( StepResult.Error() );
    }

    if ( Hierarchy.IsInitialized() and Hierarchy.UpdateWorld() )
    {
        ApplyHierarchyWorldTransforms();
    }

    Registry.Publish();

    if ( IsRenderExtractionEnabled() )
    {
        static_cast<void>( PublishRenderSnapshot() );
    }

    return Result;
}

Lumen::TExpected<void, Lumen::EReplicationError>
Lumen::FWorld::InitializeReplication ( IReplicationTransport &InTransport, IReplicationActorHandler &InHandler ) noexcept
{
    if ( Desc.ReplicationConfig.MaxActors == 0ULL )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::InvalidCapacity };
    }
    if ( Replication )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::AlreadyInitialized };
    }
    TUniquePtr<FReplicationSystem> System = MakeUnique<FReplicationSystem>();
    TExpected<void, EReplicationError> Initialized =
        System->Initialize( Desc.ReplicationConfig, Desc.ReplicationRole, InTransport, InHandler );
    if ( not Initialized )
    {
        return TUnexpected<EReplicationError>{ Initialized.Error() };
    }
    Replication = std::move( System );
    return {};
}

Lumen::FReplicationSystem *Lumen::FWorld::GetReplicationSystem () const noexcept
{
    return Replication.Get();
}

Lumen::Bool Lumen::FWorld::IsReplicationEnabled () const noexcept
{
    return Replication.IsValid() and Replication->IsInitialized();
}

Lumen::FWorldPartition *Lumen::FWorld::GetPartition () const noexcept
{
    return Partition.Get();
}

Lumen::Bool Lumen::FWorld::IsPartitionEnabled () const noexcept
{
    return Partition.IsValid() and Partition->IsInitialized();
}

Lumen::TExpected<void, Lumen::EWorldPartitionError> Lumen::FWorld::TryAssignActorToCell (
    const ActorID InActorId, const FWorldCellId InCellId, const FPersistentActorId InPersistentId ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EWorldPartitionError::WorldClosed );
    }
    if ( not IsPartitionEnabled() )
    {
        return MakeUnexpected( EWorldPartitionError::NotInitialized );
    }
    if ( IsInParallelPhase() )
    {
        return MakeUnexpected( EWorldPartitionError::ControlPhaseRequired );
    }
    if ( not Actors.contains( InActorId ) )
    {
        return MakeUnexpected( EWorldPartitionError::UnknownActor );
    }
    /**
     * INFO: Validate every non-label failure before the authority allocates a persistent ID. Replication has no
     *       unbind API, so a later cell/capacity/journal rejection would leak an irreversible binding.
     */
    LUMEN_TRY_EXPECT_ERROR( Partition->CanAssignActor( InActorId, InCellId ) );

    FPersistentActorId Label = InPersistentId;
    if ( IsReplicationEnabled() )
    {
        const EActorRole Role = Replication->GetRole();
        if ( Role == EActorRole::Authority )
        {
            /** INFO: Reject a conflicting caller label before allocation so a failed call has no replication side effect. */
            const TOptional<FPersistentActorId> ExistingLabel = Replication->FindPersistentId( InActorId );
            if ( Label.IsValid() and ( not ExistingLabel or Label != *ExistingLabel ) )
            {
                return MakeUnexpected( EWorldPartitionError::PersistentConflict );
            }
            if ( not ExistingLabel )
            {
                const TExpected<void, EReplicationError> Allocated = Replication->AssignPersistentId( InActorId );
                if ( not Allocated )
                {
                    return MakeUnexpected( Allocated.Error() == EReplicationError::CapacityExceeded
                                               ? EWorldPartitionError::CapacityExceeded
                                               : EWorldPartitionError::ReplicationRejected );
                }
            }
            const TOptional<FPersistentActorId> AuthorityLabel = Replication->FindPersistentId( InActorId );
            if ( not AuthorityLabel )
            {
                return MakeUnexpected( EWorldPartitionError::ReplicationRejected );
            }
            Label = *AuthorityLabel;
        }
        else if ( Role == EActorRole::Replica )
        {
            if ( not Label.IsValid() )
            {
                return MakeUnexpected( EWorldPartitionError::InvalidPersistentId );
            }
            const TOptional<FPersistentActorId> Bound = Replication->FindPersistentId( InActorId );
            if ( not Bound or Label != *Bound )
            {
                return MakeUnexpected( EWorldPartitionError::PersistentConflict );
            }
        }
    }
    else if ( not Label.IsValid() )
    {
        return MakeUnexpected( EWorldPartitionError::InvalidPersistentId );
    }

    TExpected<void, EWorldPartitionError> Assigned = Partition->TryAssignActor( InActorId, InCellId, Label );
    if ( not Assigned )
    {
        return Assigned;
    }
    /** INFO: New cells start unloaded, so assignment alone must close routing until the cell is loaded. */
    if ( IsActorDormant( InActorId ) )
    {
        const auto ActorFound = Actors.find( InActorId );
        if ( ActorFound != Actors.end() )
        {
            Internal::FActorRegistry::CloseRouting( ActorFound->second.Get() );
        }
    }
    return {};
}

Lumen::TExpected<void, Lumen::EWorldPartitionError> Lumen::FWorld::TryUnassignActorFromCell ( const ActorID InActorId ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EWorldPartitionError::WorldClosed );
    }
    if ( not IsPartitionEnabled() )
    {
        return MakeUnexpected( EWorldPartitionError::NotInitialized );
    }
    if ( IsInParallelPhase() )
    {
        return MakeUnexpected( EWorldPartitionError::ControlPhaseRequired );
    }
    const Bool bWasDormant                        = IsActorDormant( InActorId );
    TExpected<void, EWorldPartitionError> Removed = Partition->TryUnassignActor( InActorId );
    if ( not Removed )
    {
        return Removed;
    }
    /** INFO: Unassignment makes the actor non-dormant again; restore the routing closed on its behalf. */
    if ( bWasDormant )
    {
        const auto ActorFound = Actors.find( InActorId );
        if ( ActorFound != Actors.end() )
        {
            Internal::FActorRegistry::OpenRouting( ActorFound->second.Get() );
        }
    }
    return {};
}

Lumen::TExpected<void, Lumen::EWorldPartitionError> Lumen::FWorld::TryPartitionLoadCell ( const FWorldCellId InCellId ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EWorldPartitionError::WorldClosed );
    }
    if ( not IsPartitionEnabled() )
    {
        return MakeUnexpected( EWorldPartitionError::NotInitialized );
    }
    if ( IsInParallelPhase() )
    {
        if ( not QueueDeferredCommand( InvalidActorID, FDeferredPartitionLoadCommand{ .CellId = InCellId } ) )
        {
            return MakeUnexpected( EWorldPartitionError::DeferredQueueFull );
        }
        return {};
    }
    return ApplyPartitionLoadCell( InCellId, 0ULL );
}

Lumen::TExpected<void, Lumen::EWorldPartitionError> Lumen::FWorld::TryPartitionUnloadCell ( const FWorldCellId InCellId ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EWorldPartitionError::WorldClosed );
    }
    if ( not IsPartitionEnabled() )
    {
        return MakeUnexpected( EWorldPartitionError::NotInitialized );
    }
    if ( IsInParallelPhase() )
    {
        if ( not QueueDeferredCommand( InvalidActorID, FDeferredPartitionUnloadCommand{ .CellId = InCellId } ) )
        {
            return MakeUnexpected( EWorldPartitionError::DeferredQueueFull );
        }
        return {};
    }
    return ApplyPartitionUnloadCell( InCellId, 0ULL );
}

Lumen::Bool Lumen::FWorld::IsActorDormant ( const ActorID InActorId ) const noexcept
{
    return IsPartitionEnabled() and Partition->IsActorDormant( InActorId );
}

void Lumen::FWorld::TickReplication () noexcept
{
    if ( not IsReplicationEnabled() )
    {
        return;
    }

    /** INFO: Built in the serial control phase, before hierarchy UpdateWorld and spatial publication. */
    ReplicationSourceScratch.clear();
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        if ( Actor->GetLifecycle() != EActorLifecycle::Active )
        {
            continue;
        }
        const ActorID LocalId = Actor->GetId();
        /**
         * INFO: Dormancy is filtered only at snapshot extraction. Keeping bound actors in the replication source
         *       view prevents M9 from emitting a Destroy tombstone once the state was acked by a replica; replicated
         *       writes still reach the hierarchy and become visible when the cell loads.
         */
        FReplicationActorSourceRecord Record;
        Record.LocalId        = LocalId;
        Record.ParentLocalId  = Hierarchy.GetParentId( LocalId ).ValueOr( InvalidActorID );
        Record.LocalTransform = Hierarchy.GetLocalTransform( LocalId ).ValueOr( Maths::FMat4x4f::Identity() );
        ReplicationSourceScratch.push_back( Record );
    }

    const FReplicationSourceView View{
        .Actors = TSpan<const FReplicationActorSourceRecord>( ReplicationSourceScratch.data(), ReplicationSourceScratch.size() ),
        .FrameIndex = FrameIndex };
    static_cast<void>( Replication->Tick( FrameIndex, View ) );
}

void Lumen::FWorld::SetIngressJournal ( const FWorldIngressJournal *InJournal ) noexcept
{
    IngressJournal    = InJournal;
    IngressCursor     = 0ULL;
    IngressGeneration = InJournal != nullptr ? InJournal->GetGeneration() : 0ULL;
    if ( InJournal != nullptr )
    {
        if ( IngressBatch.capacity() < InJournal->GetCapacity() )
        {
            IngressBatch.reserve( InJournal->GetCapacity() );
        }
        IngressBatch.resize( IngressBatch.capacity() );
    }
}

Lumen::UInt64 Lumen::FWorld::GetLastIngressRecordCount () const noexcept
{
    return LastIngressRecordCount;
}

Lumen::UInt64 Lumen::FWorld::GetFrameIndex () const noexcept
{
    return FrameIndex;
}

void Lumen::FWorld::ApplyIngressJournal () noexcept
{
    LastIngressRecordCount = 0ULL;
    if ( IngressJournal == nullptr )
    {
        return;
    }

    const UInt64 Generation = IngressJournal->GetGeneration();
    if ( Generation != IngressGeneration )
    {
        IngressCursor     = 0ULL;
        IngressGeneration = Generation;
    }

    const USize Count = IngressJournal->CopyReadyRecords(
        FrameIndex, IngressCursor, TSpan<FWorldIngressRecord>( IngressBatch.data(), IngressBatch.size() ) );

    for ( USize Index = 0ULL; Index < Count; ++Index )
    {
        const FWorldIngressRecord &Record = IngressBatch[Index];
        switch ( Record.Kind )
        {
        case EWorldIngressKind::Message:
            static_cast<void>(
                Internal::FActorRegistry::TryEnqueueMessage( Record.Target, Record.Message, FrameIndex, Record.Sequence ) );
            break;
        case EWorldIngressKind::Broadcast:
            for ( const TSharedPtr<AActor> &Actor : ActiveActors )
            {
                if ( Actor->GetLifecycle() != EActorLifecycle::Active or IsActorDormant( Actor->GetId() ) )
                {
                    continue;
                }
                static_cast<void>(
                    Internal::FActorRegistry::TryEnqueueMessage( Actor->GetId(), Record.Message, FrameIndex, Record.Sequence ) );
            }
            break;
        case EWorldIngressKind::Destroy:
            static_cast<void>( TryDestroyActor( Record.Target ) );
            break;
        }

        ++IngressCursor;
        ++LastIngressRecordCount;
    }
}

Lumen::Bool Lumen::FWorld::IsInParallelPhase () const noexcept
{
    return bInParallelPhase;
}

Lumen::FWorldPhaseCompletion Lumen::FWorld::GetLastPhaseCompletion () const noexcept
{
    return LastPhase;
}

Lumen::FSpatialRegistry &Lumen::FWorld::GetSpatialRegistry () noexcept
{
    return Registry;
}

Lumen::FSceneHierarchy &Lumen::FWorld::GetHierarchy () noexcept
{
    return Hierarchy;
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError>
Lumen::FWorld::TryRegisterHierarchyActor ( const ActorID InActorId, const Maths::FMat4x4f &InLocalTransform ) noexcept
{
    if ( not Hierarchy.IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::NotInitialized );
    }

    if ( IsInParallelPhase() )
    {
        if ( not QueueDeferredCommand( InActorId,
                                       FDeferredHierarchyRegisterCommand{ .ActorId = InActorId, .Local = InLocalTransform } ) )
        {
            return MakeUnexpected( ESceneHierarchyError::DeferredQueueFull );
        }
        return {};
    }

    return ApplyHierarchyRegister( InActorId, InLocalTransform );
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError> Lumen::FWorld::TryHierarchyAttach ( const ActorID InActorId,
                                                                                        const ActorID InParentId ) noexcept
{
    if ( not Hierarchy.IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::NotInitialized );
    }

    if ( IsInParallelPhase() )
    {
        if ( not QueueDeferredCommand( InActorId,
                                       FDeferredHierarchyAttachCommand{ .ActorId = InActorId, .ParentId = InParentId } ) )
        {
            return MakeUnexpected( ESceneHierarchyError::DeferredQueueFull );
        }
        return {};
    }

    return ApplyHierarchyAttach( InActorId, InParentId );
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError> Lumen::FWorld::TryHierarchyDetach ( const ActorID InActorId ) noexcept
{
    if ( not Hierarchy.IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::NotInitialized );
    }

    if ( IsInParallelPhase() )
    {
        if ( not QueueDeferredCommand( InActorId, FDeferredHierarchyDetachCommand{ InActorId } ) )
        {
            return MakeUnexpected( ESceneHierarchyError::DeferredQueueFull );
        }
        return {};
    }

    return ApplyHierarchyDetach( InActorId );
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError>
Lumen::FWorld::TrySetHierarchyLocalTransform ( const ActorID InActorId, const Maths::FMat4x4f &InLocalTransform ) noexcept
{
    if ( not Hierarchy.IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::NotInitialized );
    }

    if ( IsInParallelPhase() )
    {
        if ( not QueueDeferredCommand(
                 InActorId, FDeferredHierarchyLocalTransformCommand{ .ActorId = InActorId, .Local = InLocalTransform } ) )
        {
            return MakeUnexpected( ESceneHierarchyError::DeferredQueueFull );
        }
        return {};
    }

    return ApplyHierarchyLocalTransform( InActorId, InLocalTransform );
}

Lumen::UInt64 Lumen::FWorld::GetWorldId () const noexcept
{
    return WorldId;
}

Lumen::Bool Lumen::FWorld::IsRenderExtractionEnabled () const noexcept
{
    const FRenderSnapshotCapacities &Capacities = Desc.RenderCapacities;
    return Capacities.TransformCapacity != 0ULL or Capacities.MeshCapacity != 0ULL or Capacities.CameraCapacity != 0ULL or
           Capacities.LightCapacity != 0ULL;
}

Lumen::FRenderSnapshotStream &Lumen::FWorld::GetRenderSnapshotStream () noexcept
{
    return RenderSnapshotStream;
}

Lumen::TExpected<Lumen::FRenderSnapshotHeader, Lumen::FRenderSnapshotError> Lumen::FWorld::PublishRenderSnapshot () noexcept
{
    const FSpatialCommittedView Committed       = Registry.GetCommittedView();
    const FRenderSnapshotCapacities &Capacities = Desc.RenderCapacities;

    const auto Fail =
        [this] ( const FRenderSnapshotError &InError ) noexcept -> TExpected<FRenderSnapshotHeader, FRenderSnapshotError>
    {
        LastRenderError = InError;
        bHasRenderError = true;
        return MakeUnexpected( LastRenderError );
    };

    USize VisibleEntityCount = 0ULL;
    for ( const ActorID Entity : Committed.EntityIDs )
    {
        if ( not IsActorDormant( Entity ) )
        {
            ++VisibleEntityCount;
        }
    }
    USize VisibleMeshCount   = 0ULL;
    USize VisibleCameraCount = 0ULL;
    USize VisibleLightCount  = 0ULL;
    for ( const FRenderMeshRecord &Mesh : RenderContent.Meshes )
    {
        if ( not IsRenderProxyDormant( Mesh.ProxyId ) )
        {
            ++VisibleMeshCount;
        }
    }
    for ( const FRenderCameraRecord &Camera : RenderContent.Cameras )
    {
        if ( not IsRenderProxyDormant( Camera.ProxyId ) )
        {
            ++VisibleCameraCount;
        }
    }
    for ( const FRenderLightRecord &Light : RenderContent.Lights )
    {
        if ( not IsRenderProxyDormant( Light.ProxyId ) )
        {
            ++VisibleLightCount;
        }
    }

    /** INFO: Validate filtered counts before touching any scratch array so an oversized input cannot force an allocation. */
    if ( VisibleEntityCount > Capacities.TransformCapacity ) [[unlikely]]
    {
        return Fail(
            MakeCapacityError( ERenderSnapshotRecordType::Transform, VisibleEntityCount, Capacities.TransformCapacity ) );
    }
    if ( VisibleMeshCount > Capacities.MeshCapacity ) [[unlikely]]
    {
        return Fail( MakeCapacityError( ERenderSnapshotRecordType::Mesh, VisibleMeshCount, Capacities.MeshCapacity ) );
    }
    if ( VisibleCameraCount > Capacities.CameraCapacity ) [[unlikely]]
    {
        return Fail( MakeCapacityError( ERenderSnapshotRecordType::Camera, VisibleCameraCount, Capacities.CameraCapacity ) );
    }
    if ( VisibleLightCount > Capacities.LightCapacity ) [[unlikely]]
    {
        return Fail( MakeCapacityError( ERenderSnapshotRecordType::Light, VisibleLightCount, Capacities.LightCapacity ) );
    }

    RenderTransforms.clear();
    for ( USize Index = 0ULL; Index < Committed.EntityIDs.size(); ++Index )
    {
        const ActorID Proxy = Committed.EntityIDs[Index];
        if ( IsActorDormant( Proxy ) )
        {
            continue;
        }
        ERenderTransformFlags Flags = ERenderTransformFlags::Visible;
        if ( IsReplicationEnabled() )
        {
            const TOptional<FPersistentActorId> Persistent = Replication->FindPersistentId( Proxy );
            if ( Persistent and not Replication->GetVisibility( *Persistent ) )
            {
                Flags = ERenderTransformFlags::None;
            }
        }
        RenderTransforms.emplace_back( FRenderTransformRecord{
            .ProxyId        = FRenderProxyId{ Proxy },
            .WorldTransform = Committed.Transforms[Index],
            .Bounds         = FRenderBounds{ .Min = Maths::FVec3f{ -1.0F }, .Max = Maths::FVec3f{ 1.0F } },
            .Flags          = Flags,
        } );
    }
    Algo::Sort( RenderTransforms, &SortRenderTransforms );

    RenderMeshes.clear();
    for ( const FRenderMeshRecord &Mesh : RenderContent.Meshes )
    {
        if ( IsRenderProxyDormant( Mesh.ProxyId ) )
        {
            continue;
        }
        FRenderMeshRecord Remapped = Mesh;
        Remapped.TransformIndex    = FindProxyRank( RenderTransforms, Mesh.ProxyId );
        RenderMeshes.emplace_back( Remapped );
    }

    RenderCameras.clear();
    for ( const FRenderCameraRecord &Camera : RenderContent.Cameras )
    {
        if ( not IsRenderProxyDormant( Camera.ProxyId ) )
        {
            RenderCameras.push_back( Camera );
        }
    }
    RenderLights.clear();
    for ( const FRenderLightRecord &Light : RenderContent.Lights )
    {
        if ( not IsRenderProxyDormant( Light.ProxyId ) )
        {
            RenderLights.push_back( Light );
        }
    }

    const FSceneExtractionInput Input{
        .WorldId    = WorldId,
        .FrameIndex = FrameIndex,
        .Transforms = TSpan<const FRenderTransformRecord>{ RenderTransforms.data(), RenderTransforms.size() },
        .Meshes     = TSpan<const FRenderMeshRecord>{ RenderMeshes.data(), RenderMeshes.size() },
        .Cameras    = TSpan<const FRenderCameraRecord>{ RenderCameras.data(), RenderCameras.size() },
        .Lights     = TSpan<const FRenderLightRecord>{ RenderLights.data(), RenderLights.size() },
    };

    auto Result = RenderExtractor.Publish( Input );
    if ( not Result ) [[unlikely]]
    {
        LastRenderError = Result.Error();
        bHasRenderError = true;
        return MakeUnexpected( LastRenderError );
    }

    LastRenderError = {};
    bHasRenderError = false;
    ++RenderPublishCount;
    return Result;
}

Lumen::Bool Lumen::FWorld::SwapRenderSnapshotReadBuffers () noexcept
{
    return RenderSnapshotStream.SwapReadBuffers();
}

Lumen::FRenderSnapshotView Lumen::FWorld::GetRenderSnapshot () const noexcept
{
    return RenderSnapshotStream.GetReadSnapshot();
}

Lumen::TOptional<Lumen::FRenderSnapshotError> Lumen::FWorld::GetLastRenderError () const noexcept
{
    return bHasRenderError ? TOptional<FRenderSnapshotError>( LastRenderError ) : TOptional<FRenderSnapshotError>{};
}

Lumen::UInt64 Lumen::FWorld::GetRenderPublishCount () const noexcept
{
    return RenderPublishCount;
}

Lumen::FWorldRenderContent &Lumen::FWorld::GetRenderContent () noexcept
{
    return RenderContent;
}

Lumen::TExpected<void, Lumen::EActorSpawnError> Lumen::FWorld::InsertSpawnedActor ( const TSharedPtr<AActor> &InActor ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EActorSpawnError::WorldClosed );
    }
    if ( not InActor.IsValid() or not InActor->bRegistered )
    {
        return MakeUnexpected( EActorSpawnError::RegistrationFailed );
    }
    if ( Actors.size() >= Desc.MaxActors )
    {
        return MakeUnexpected( EActorSpawnError::WorldCapacityExceeded );
    }

    const ActorID NewId = InActor->GetId();
    if ( not Actors.emplace( NewId, InActor ).second )
    {
        return MakeUnexpected( EActorSpawnError::RegistrationFailed );
    }

    ActiveActors.push_back( InActor );
    bNeedsSorting = true;
    InActor->BeginPlay();
    return {};
}

Lumen::Bool Lumen::FWorld::QueueDeferredCommand ( const ActorID InTarget, FDeferredCommandPayload &&InPayload ) noexcept
{
    FActorEmitContext *const EmitContext = AActor::GetCurrentEmitContext();

    FDeferredCommand Command;
    Command.Target  = InTarget;
    Command.Payload = std::move( InPayload );
    if ( EmitContext != nullptr )
    {
        Command.Sender         = EmitContext->Sender;
        Command.SenderSequence = EmitContext->AllocateCommandSequence();
    }

    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( DeferredMutex );
    if ( bShuttingDown or DeferredCommands.size() >= Desc.DeferredCommandCapacity )
    {
        return false;
    }

    if ( EmitContext == nullptr )
    {
        Command.Sender         = InvalidActorID;
        Command.SenderSequence = NextExternalCommandSequence++;
    }
    DeferredCommands.push_back( std::move( Command ) );
    return true;
}

Lumen::TExpected<void, Lumen::EActorDestroyError> Lumen::FWorld::DestroyActorImpl ( const ActorID InId ) noexcept
{
    const auto ActorIterator = Actors.find( InId );
    if ( ActorIterator == Actors.end() )
    {
        return MakeUnexpected( EActorDestroyError::UnknownActor );
    }

    TSharedPtr<AActor> Actor = ActorIterator->second;
    if ( Actor->GetLifecycle() != EActorLifecycle::Active )
    {
        return MakeUnexpected( EActorDestroyError::ActorRetiring );
    }

    /**
     * INFO: Refuse destruction while the partition journal cannot record the unassignment; otherwise a destroyed
     *       ActorID would remain registered with stale dormancy and counters.
     */
    if ( IsPartitionEnabled() )
    {
        const TExpected<void, EWorldPartitionError> CanRemove = Partition->CanUnassignActor( InId );
        /** INFO: ActorNotAssigned is the normal case for opt-in cells; only journal pressure can block a destroy. */
        if ( not CanRemove and CanRemove.Error() == EWorldPartitionError::JournalFull )
        {
            return MakeUnexpected( EActorDestroyError::PartitionJournalFull );
        }
    }

    Actor->MarkPendingKill();
    Registry.UnregisterSpatialEntity( InId );
    if ( IsPartitionEnabled() )
    {
        static_cast<void>( Partition->TryUnassignActor( InId ) );
    }
    if ( Hierarchy.IsInitialized() )
    {
        /** INFO: Default destruction policy detaches children; the hierarchy never owns or cascades actor lifetimes. */
        static_cast<void>( Hierarchy.Unregister( InId ) );
    }

    Actors.erase( ActorIterator );
    for ( auto ActiveIterator = ActiveActors.begin(); ActiveIterator != ActiveActors.end(); ++ActiveIterator )
    {
        if ( ( *ActiveIterator )->GetId() == InId )
        {
            ActiveActors.erase( ActiveIterator );
            break;
        }
    }

    Actor->EndPlay();
    Actor->Retire();
    bNeedsSorting = true;
    return {};
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError>
Lumen::FWorld::ApplyHierarchyRegister ( const ActorID InActorId, const Maths::FMat4x4f &InLocalTransform ) noexcept
{
    if ( not Actors.contains( InActorId ) )
    {
        return MakeUnexpected( ESceneHierarchyError::UnknownActor );
    }
    return Hierarchy.Register( InActorId, InLocalTransform );
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError> Lumen::FWorld::ApplyHierarchyAttach ( const ActorID InActorId,
                                                                                          const ActorID InParentId ) noexcept
{
    if ( InParentId != InvalidActorID and not Actors.contains( InParentId ) )
    {
        return MakeUnexpected( ESceneHierarchyError::StaleParent );
    }
    return Hierarchy.Attach( InActorId, InParentId );
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError> Lumen::FWorld::ApplyHierarchyDetach ( const ActorID InActorId ) noexcept
{
    return Hierarchy.Detach( InActorId );
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError>
Lumen::FWorld::ApplyHierarchyLocalTransform ( const ActorID InActorId, const Maths::FMat4x4f &InLocalTransform ) noexcept
{
    return Hierarchy.SetLocalTransform( InActorId, InLocalTransform );
}

void Lumen::FWorld::ApplyHierarchyWorldTransforms () noexcept
{
    for ( const FSceneHierarchyTransformRecord &Record : Hierarchy.GetPublishedTransforms() )
    {
        Registry.UpdateTransform( Record.ActorId, Record.WorldTransform );
    }
}

Lumen::TExpected<void, Lumen::EWorldPartitionError>
Lumen::FWorld::ApplyPartitionLoadCell ( const FWorldCellId InCellId, const UInt64 InSenderSequence ) noexcept
{
    if ( not IsPartitionEnabled() )
    {
        return MakeUnexpected( EWorldPartitionError::NotInitialized );
    }
    LUMEN_TRY_EXPECT_ERROR( Partition->TryLoadCell( InCellId, FrameIndex, InSenderSequence ) );
    SetCellActorRouting( InCellId, true );
    return {};
}

Lumen::TExpected<void, Lumen::EWorldPartitionError>
Lumen::FWorld::ApplyPartitionUnloadCell ( const FWorldCellId InCellId, const UInt64 InSenderSequence ) noexcept
{
    if ( not IsPartitionEnabled() )
    {
        return MakeUnexpected( EWorldPartitionError::NotInitialized );
    }
    LUMEN_TRY_EXPECT_ERROR( Partition->TryUnloadCell( InCellId, FrameIndex, InSenderSequence ) );
    DetachExternalChildrenOfCell( InCellId );
    SetCellActorRouting( InCellId, false );
    return {};
}

void Lumen::FWorld::DetachExternalChildrenOfCell ( const FWorldCellId InCellId ) noexcept
{
    if ( not Hierarchy.IsInitialized() )
    {
        return;
    }
    PartitionScratch.clear();
    const TSpan<ActorID> Registered( PartitionScratch.data(), PartitionScratch.capacity() );
    const USize Count = Hierarchy.GetRegisteredActorIds( Registered );
    /** INFO: Working nodes are authoritative even before the next publication; only external children are detached. */
    for ( USize Index = 0ULL; Index < Count; ++Index )
    {
        const ActorID Child             = PartitionScratch[Index];
        const TOptional<ActorID> Parent = Hierarchy.GetParentId( Child );
        if ( not Parent )
        {
            continue;
        }
        const TOptional<FWorldCellId> ParentCell = Partition->FindActorCell( *Parent );
        if ( not ParentCell or *ParentCell != InCellId )
        {
            continue;
        }
        const TOptional<FWorldCellId> ChildCell = Partition->FindActorCell( Child );
        if ( ChildCell and *ChildCell == InCellId )
        {
            continue;
        }
        static_cast<void>( Hierarchy.Detach( Child ) );
    }
}

void Lumen::FWorld::SetCellActorRouting ( const FWorldCellId InCellId, const Bool bInRoutable ) noexcept
{
    PartitionScratch.clear();
    const TSpan<ActorID> Candidates( PartitionScratch.data(), PartitionScratch.capacity() );
    const USize Count = Partition->GetCellActors( InCellId, Candidates );
    for ( USize Index = 0ULL; Index < Count; ++Index )
    {
        const auto ActorFound = Actors.find( PartitionScratch[Index] );
        if ( ActorFound == Actors.end() )
        {
            continue;
        }
        if ( bInRoutable )
        {
            Internal::FActorRegistry::OpenRouting( ActorFound->second.Get() );
        }
        else
        {
            Internal::FActorRegistry::CloseRouting( ActorFound->second.Get() );
        }
    }
}

Lumen::Bool Lumen::FWorld::IsRenderProxyDormant ( const FRenderProxyId InProxyId ) const noexcept
{
    if ( not IsPartitionEnabled() )
    {
        return false;
    }
    const ActorID Proxy = InProxyId.GetValue();
    return Partition->IsActorDormant( Proxy );
}

Lumen::TExpected<Lumen::FWorldPhaseCompletion, Lumen::EWorldTickError>
Lumen::FWorld::RunPhase ( const TSpan<AActor *const> InActors,
                          const EActorTickPhase InPhase,
                          const UInt64 InFixedStepIndex,
                          const Float64 InDeltaTime ) noexcept
{
    FWorldFrameContext Context{
        .FrameIndex = FrameIndex, .FixedStepIndex = InFixedStepIndex, .TaskCount = InActors.size(), .Phase = InPhase };

    bInParallelPhase = not InActors.empty();
    TExpected<FWorldPhaseCompletion, EWorldTickError> Completion =
        Executor->ExecutePhase( InActors, Context, InDeltaTime, MaxMessagesPerTick );
    bInParallelPhase = false;
    return Completion;
}

Lumen::TExpected<void, Lumen::EWorldTickError>
Lumen::FWorld::RunFixedStep ( const UInt64 InFixedStepIndex, const Float64 InDeltaTime, FWorldTickResult &OutResult ) noexcept
{
    PhaseActors.clear();
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        if ( Actor->GetLifecycle() == EActorLifecycle::Active and Actor->GetTickPhase() == EActorTickPhase::FixedUpdate and
             not IsActorDormant( Actor->GetId() ) )
        {
            PhaseActors.push_back( Actor.Get() );
            Actor->BeginTick();
        }
    }

    TExpected<FWorldPhaseCompletion, EWorldTickError> Completion =
        RunPhase( TSpan<AActor *const>( PhaseActors.data(), PhaseActors.size() ), EActorTickPhase::FixedUpdate, InFixedStepIndex,
                  InDeltaTime );

    for ( AActor *Actor : PhaseActors )
    {
        Actor->EndTick();
    }

    if ( not Completion )
    {
        return MakeUnexpected( Completion.Error() );
    }

    OutResult.DispatchedTaskCount += Completion->CompletedTaskCount;
    LastPhase = *Completion;
    if ( not MergeEmissions( TSpan<AActor *const>( PhaseActors.data(), PhaseActors.size() ), OutResult ) )
    {
        return MakeUnexpected( EWorldTickError::EmissionCapacityExceeded );
    }
    return {};
}

Lumen::Bool Lumen::FWorld::MergeEmissions ( const TSpan<AActor *const> InActors, FWorldTickResult &OutResult ) noexcept
{
    MergedEmissions.clear();
    Bool bCapacityExceeded = false;

    for ( AActor *Actor : InActors )
    {
        for ( const FActorEmission &Emission : Actor->EmitContext.GetEmissions() )
        {
            if ( MergedEmissions.size() >= Desc.EmissionCapacity )
            {
                Internal::FActorRegistry::RecordDeadLetter( EActorSendError::OutboxFull, Emission.Target, Emission.Message,
                                                            FrameIndex, Emission.SenderSequence );
                bCapacityExceeded = true;
                continue;
            }
            MergedEmissions.push_back( Emission );
        }
        Actor->EmitContext.Emissions.clear();
    }

    Algo::Sort( MergedEmissions, &SortEmissions );
    for ( const FActorEmission &Emission : MergedEmissions )
    {
        TExpected<void, EActorSendError> Routed =
            Internal::FActorRegistry::TryEnqueueMessage( Emission.Target, Emission.Message, FrameIndex, Emission.SenderSequence );
        if ( Routed )
        {
            ++OutResult.RoutedMessageCount;
        }
    }
    return not bCapacityExceeded;
}

Lumen::Bool Lumen::FWorld::ApplyDeferredCommands () noexcept
{
    DeferredCommandBatch.clear();
    {
        HAL::TUniqueLock<HAL::FPlatformMutex> Lock( DeferredMutex );
        DeferredCommandBatch.swap( DeferredCommands );
    }

    Algo::Sort( DeferredCommandBatch, [] ( const FDeferredCommand &InLeft, const FDeferredCommand &InRight ) noexcept -> Bool
    {
        if ( InLeft.Sender != InRight.Sender )
        {
            return InLeft.Sender < InRight.Sender;
        }
        if ( InLeft.SenderSequence != InRight.SenderSequence )
        {
            return InLeft.SenderSequence < InRight.SenderSequence;
        }
        if ( InLeft.Target != InRight.Target )
        {
            return InLeft.Target < InRight.Target;
        }
        return InLeft.Payload.index() < InRight.Payload.index();
    } );

    Bool bWorldCapacityExceeded = false;
    for ( FDeferredCommand &Command : DeferredCommandBatch )
    {
        std::visit( Meta::FOverloaded{ [this, &bWorldCapacityExceeded] ( FDeferredSpawnCommand &InSpawn ) noexcept
        {
            TExpected<void, EActorSpawnError> Inserted = InsertSpawnedActor( InSpawn.Actor );
            if ( not Inserted and Inserted.Error() == EActorSpawnError::WorldCapacityExceeded )
            {
                bWorldCapacityExceeded = true;
            }
        },
                                       [this] ( FDeferredDestroyCommand &InDestroy ) noexcept
        {
            static_cast<void>( DestroyActorImpl( InDestroy.Target ) );
        },
                                       [this] ( FDeferredBroadcastCommand &InBroadcast ) noexcept
        {
            static_cast<void>( TryBroadcast( InBroadcast.Message ) );
        },
                                       [this] ( FDeferredHierarchyRegisterCommand &InRegister ) noexcept
        {
            static_cast<void>( ApplyHierarchyRegister( InRegister.ActorId, InRegister.Local ) );
        },
                                       [this] ( FDeferredHierarchyAttachCommand &InAttach ) noexcept
        {
            static_cast<void>( ApplyHierarchyAttach( InAttach.ActorId, InAttach.ParentId ) );
        },
                                       [this] ( FDeferredHierarchyDetachCommand &InDetach ) noexcept
        {
            static_cast<void>( ApplyHierarchyDetach( InDetach.ActorId ) );
        },
                                       [this] ( FDeferredHierarchyLocalTransformCommand &InTransform ) noexcept
        {
            static_cast<void>( ApplyHierarchyLocalTransform( InTransform.ActorId, InTransform.Local ) );
        },
                                       [this, &Command] ( FDeferredPartitionLoadCommand &InLoad ) noexcept
        {
            static_cast<void>( ApplyPartitionLoadCell( InLoad.CellId, Command.SenderSequence ) );
        },
                                       [this, &Command] ( FDeferredPartitionUnloadCommand &InUnload ) noexcept
        {
            static_cast<void>( ApplyPartitionUnloadCell( InUnload.CellId, Command.SenderSequence ) );
        } },
                    Command.Payload );
    }

    DeferredCommandBatch.clear();
    return bWorldCapacityExceeded;
}

void Lumen::FWorld::SortActorsIfNeeded () noexcept
{
    if ( bNeedsSorting )
    {
        Algo::Sort( ActiveActors, &SortActiveActors );
        bNeedsSorting = false;
    }
}

void Lumen::FWorld::ShutdownActors () noexcept
{
    const auto RetirePendingSpawns = [] ( TVector<FDeferredCommand> &InCommands ) noexcept
    {
        for ( FDeferredCommand &Command : InCommands )
        {
            if ( FDeferredSpawnCommand *const Spawn = std::get_if<FDeferredSpawnCommand>( &Command.Payload ) )
            {
                if ( Spawn->Actor.IsValid() )
                {
                    Spawn->Actor->Retire();
                }
            }
        }
    };

    RetirePendingSpawns( DeferredCommands );
    RetirePendingSpawns( DeferredCommandBatch );

    Algo::ForEach( ActiveActors, [&] ( const TSharedPtr<AActor> &Actor ) noexcept -> void
    {
        Registry.UnregisterSpatialEntity( Actor->GetId() );
        Actor->MarkPendingKill();
        Actor->EndPlay();
    } );

    Algo::ForEach( ActiveActors, [] ( const TSharedPtr<AActor> &Actor ) noexcept -> void
    {
        Actor->Retire();
    } );

    if ( Partition.IsValid() )
    {
        Partition->Clear();
    }
    Actors.clear();
    ActiveActors.clear();
    DeferredCommands.clear();
    DeferredCommandBatch.clear();
}
