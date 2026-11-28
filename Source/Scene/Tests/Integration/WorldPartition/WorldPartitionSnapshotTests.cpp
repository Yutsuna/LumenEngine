/**
 * @file WorldPartitionSnapshotTests.cpp
 * @brief Unloaded cells are absent from snapshots, routing, hierarchy and sibling worlds.
 */

#include <World/World.hpp>
#include <World/WorldIngressJournal.hpp>
#include <World/WorldSpatialRegistry.hpp>
#include <World/WorldTypes.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>
#include <Camera/CameraTypes.hpp>
#include <Hierarchy/SceneHierarchy.hpp>
#include <Rendering/RenderRecords.hpp>
#include <Rendering/RenderSnapshotView.hpp>
#include <Replication/ReplicationFrame.hpp>
#include <Replication/ReplicationSystem.hpp>
#include <Replication/ReplicationTransport.hpp>
#include <Replication/ReplicationTypes.hpp>
#include <WorldPartition/WorldPartition.hpp>

#include <Templates/Unexpected.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    [[nodiscard]] FWorldDesc MakeDesc ( const UInt64 InWorldId ) noexcept
    {
        FWorldDesc Desc;
        Desc.WorkerCount       = 2U;
        Desc.MaxActors         = 16ULL;
        Desc.HierarchyCapacity = 16ULL;
        Desc.WorldId           = InWorldId;
        Desc.PartitionConfig   = FWorldPartitionConfig{ .MaxCells = 8ULL, .MaxAssignedActors = 8ULL, .MaxJournalRecords = 32ULL };
        Desc.RenderCapacities  = FRenderSnapshotCapacities{
            .TransformCapacity = 16ULL,
            .MeshCapacity      = 16ULL,
            .CameraCapacity    = 16ULL,
            .LightCapacity     = 16ULL,
        };
        return Desc;
    }

    [[nodiscard]] TSharedRef<AActor> SpawnProbe ( FWorld &InWorld )
    {
        class FPartitionProbeActor final : public AActor
        {
            LUMEN_ACTOR_BODY ( FPartitionProbeActor );

        public:

            explicit FPartitionProbeActor ( const ActorID InId ) noexcept : AActor( InId )
            {
            }

            void Receive ( const FMessage &InMessage ) noexcept override
            {
                static_cast<void>( InMessage );
            }
        };

        auto Spawned = InWorld.TrySpawnActor<FPartitionProbeActor>();
        LUMEN_CHECK_HAS_VALUE( Spawned );
        return *Spawned;
    }

    class FCountingProbeActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FCountingProbeActor );

    public:

        FCountingProbeActor ( const ActorID InId, TAtomic<UInt32> *InCount ) noexcept : AActor( InId ), Count( InCount )
        {
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            static_cast<void>( InMessage );
            Count->fetch_add( 1U, std::memory_order_relaxed );
        }

    private:

        TAtomic<UInt32> *Count = nullptr;
    };

    void RegisterPublishedActor ( FWorld &InWorld, const TSharedRef<AActor> &InActor )
    {
        const ActorID Id = InActor->GetId();
        LUMEN_CHECK_HAS_VALUE( InWorld.GetSpatialRegistry().TryRegisterSpatialEntity( Id ) );
        LUMEN_CHECK_HAS_VALUE( InWorld.TryRegisterHierarchyActor( Id ) );
    }

    void AddRenderContent ( FWorld &InWorld, const ActorID InActorId )
    {
        const FRenderProxyId Proxy{ InActorId };
        InWorld.GetRenderContent().Meshes.emplace_back( FRenderMeshRecord{
            .ProxyId         = Proxy,
            .MeshAssetId     = FRenderAssetId{ 700000ULL + InActorId },
            .MaterialAssetId = FRenderAssetId{ 800000ULL + InActorId },
        } );
        LUMEN_CHECK_HAS_VALUE( InWorld.TryAddCameraComponent( InActorId, FCameraComponentDesc{} ) );
        InWorld.GetRenderContent().Lights.emplace_back(
            FRenderLightRecord{ .ProxyId = Proxy, .Position = Maths::FVec3f{ 0.0F } } );
    }

    [[nodiscard]] FRenderSnapshotView PublishAndSwap ( FWorld &InWorld )
    {
        LUMEN_CHECK_HAS_VALUE( InWorld.TryTick( 0.016 ) );
        LUMEN_CHECK_TRUE( InWorld.SwapRenderSnapshotReadBuffers() );
        return InWorld.GetRenderSnapshot();
    }

    class FCaptureTransport final : public IReplicationTransport
    {
    public:

        [[nodiscard]] TExpected<void, EReplicationError> TrySend ( const TSpan<const Byte> InBytes ) noexcept override
        {
            Captured.clear();
            for ( const Byte Value : InBytes )
            {
                Captured.push_back( Value );
            }
            return {};
        }

        [[nodiscard]] TExpected<USize, EReplicationError> TryReceive ( const TSpan<Byte> InBytes ) noexcept override
        {
            static_cast<void>( InBytes );
            return 0ULL;
        }

        TVector<Byte> Captured;
    };

    class FNoopReplicationHandler final : public IReplicationActorHandler
    {
    public:

        [[nodiscard]] TExpected<ActorID, EReplicationError> TrySpawn ( const FPersistentActorId InId ) noexcept override
        {
            static_cast<void>( InId );
            return TUnexpected<EReplicationError>{ EReplicationError::AuthorityViolation };
        }

        [[nodiscard]] TExpected<void, EReplicationError> TryDestroy ( const ActorID InLocalId ) noexcept override
        {
            static_cast<void>( InLocalId );
            return TUnexpected<EReplicationError>{ EReplicationError::AuthorityViolation };
        }

        [[nodiscard]] TExpected<void, EReplicationError>
        TrySetLocalTransform ( const ActorID InLocalId, const Maths::FMat4x4f &InLocalTransform ) noexcept override
        {
            static_cast<void>( InLocalId );
            static_cast<void>( InLocalTransform );
            return TUnexpected<EReplicationError>{ EReplicationError::AuthorityViolation };
        }

        [[nodiscard]] TExpected<void, EReplicationError> TrySetParent ( const ActorID InLocalId,
                                                                        const ActorID InParentLocalId ) noexcept override
        {
            static_cast<void>( InLocalId );
            static_cast<void>( InParentLocalId );
            return TUnexpected<EReplicationError>{ EReplicationError::AuthorityViolation };
        }
    };

} // namespace

LUMEN_INTEGRATION_TEST( FWorldPartitionSnapshot, UnloadRemovesAllRecordFamiliesAndSkippedPublications )
{
    FWorld World( MakeDesc( 99111ULL ) );
    const TSharedRef<AActor> Member = SpawnProbe( World );
    RegisterPublishedActor( World, Member );
    AddRenderContent( World, Member->GetId() );

    const FWorldCellId Cell{ 11ULL };
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Member->GetId(), Cell, FPersistentActorId{ 700ULL } ) );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( Cell ) );

    FRenderSnapshotView View = PublishAndSwap( World );
    LUMEN_CHECK_EQ( View.Transforms.size(), 1ULL );
    LUMEN_CHECK_EQ( View.Meshes.size(), 1ULL );
    LUMEN_CHECK_EQ( View.Cameras.size(), 1ULL );
    LUMEN_CHECK_EQ( View.Lights.size(), 1ULL );
    LUMEN_CHECK_EQ( View.Meshes[0ULL].TransformIndex, 0ULL );

    LUMEN_CHECK_HAS_VALUE( World.TryPartitionUnloadCell( Cell ) );
    LUMEN_CHECK_TRUE( World.IsActorDormant( Member->GetId() ) );
    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
    View = World.GetRenderSnapshot();
    LUMEN_CHECK_EQ( View.Transforms.size(), 0ULL );
    LUMEN_CHECK_EQ( View.Meshes.size(), 0ULL );
    LUMEN_CHECK_EQ( View.Cameras.size(), 0ULL );
    LUMEN_CHECK_EQ( View.Lights.size(), 0ULL );

    LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( Cell ) );
    View = PublishAndSwap( World );
    LUMEN_CHECK_EQ( View.Transforms.size(), 1ULL );
    LUMEN_CHECK_EQ( View.Meshes.size(), 1ULL );
    LUMEN_CHECK_EQ( View.Meshes[0ULL].TransformIndex, 0ULL );
    LUMEN_CHECK_EQ( View.Cameras.size(), 1ULL );
    LUMEN_CHECK_EQ( View.Lights.size(), 1ULL );
}

LUMEN_INTEGRATION_TEST( FWorldPartitionRouting, DormantActorIsNotRoutableAndReloadRestoresRouting )
{
    FWorld World( MakeDesc( 99112ULL ) );
    const TSharedRef<AActor> Member = SpawnProbe( World );
    const FWorldCellId Cell{ 12ULL };
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Member->GetId(), Cell, FPersistentActorId{ 701ULL } ) );

    const FMessage Message = FMessage::Make( 1U, 0ULL );
    /** INFO: A new cell starts unloaded, so assignment alone must close routing. */
    LUMEN_CHECK_HAS_ERROR( Member->GetRef().TryEnqueueMessage( Message ) );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( Cell ) );
    LUMEN_CHECK_HAS_VALUE( Member->GetRef().TryEnqueueMessage( Message ) );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionUnloadCell( Cell ) );
    LUMEN_CHECK_HAS_ERROR( Member->GetRef().TryEnqueueMessage( Message ) );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( Cell ) );
    LUMEN_CHECK_HAS_VALUE( Member->GetRef().TryEnqueueMessage( Message ) );
}

LUMEN_INTEGRATION_TEST( FWorldPartitionRouting, UnassignRestoresRoutingOfDormantActor )
{
    FWorld World( MakeDesc( 99117ULL ) );
    const TSharedRef<AActor> Member = SpawnProbe( World );
    const FWorldCellId Cell{ 16ULL };
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Member->GetId(), Cell, FPersistentActorId{ 706ULL } ) );
    LUMEN_CHECK_TRUE( World.IsActorDormant( Member->GetId() ) );

    const FMessage Message = FMessage::Make( 1U, 0ULL );
    LUMEN_CHECK_HAS_ERROR( Member->GetRef().TryEnqueueMessage( Message ) );
    LUMEN_CHECK_HAS_VALUE( World.TryUnassignActorFromCell( Member->GetId() ) );
    LUMEN_CHECK_FALSE( World.IsActorDormant( Member->GetId() ) );
    LUMEN_CHECK_HAS_VALUE( Member->GetRef().TryEnqueueMessage( Message ) );
}

LUMEN_INTEGRATION_TEST( FWorldPartitionIngress, BroadcastSkipsDormantActors )
{
    FWorld World( MakeDesc( 99118ULL ) );
    TAtomic<UInt32> ActiveCount  = { 0U };
    TAtomic<UInt32> DormantCount = { 0U };
    auto Active                  = World.TrySpawnActor<FCountingProbeActor>( &ActiveCount );
    auto Dormant                 = World.TrySpawnActor<FCountingProbeActor>( &DormantCount );
    LUMEN_CHECK_HAS_VALUE( Active );
    LUMEN_CHECK_HAS_VALUE( Dormant );

    const FWorldCellId ActiveCell{ 30ULL };
    const FWorldCellId DormantCell{ 31ULL };
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( ( *Active )->GetId(), ActiveCell, FPersistentActorId{ 710ULL } ) );
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( ( *Dormant )->GetId(), DormantCell, FPersistentActorId{ 711ULL } ) );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( ActiveCell ) );
    LUMEN_CHECK_FALSE( World.IsActorDormant( ( *Active )->GetId() ) );
    LUMEN_CHECK_TRUE( World.IsActorDormant( ( *Dormant )->GetId() ) );

    FWorldIngressJournal Journal;
    Journal.Reserve( 1ULL );
    LUMEN_CHECK_HAS_VALUE( Journal.AppendBroadcast( 1ULL, 0ULL, FMessage::Make( 9U, 0ULL ) ) );
    World.SetIngressJournal( &Journal );
    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    LUMEN_CHECK_EQ( ActiveCount.load( std::memory_order_acquire ), 1U );
    LUMEN_CHECK_EQ( DormantCount.load( std::memory_order_acquire ), 0U );
}

LUMEN_INTEGRATION_TEST( FWorldPartitionHierarchy, UnloadDetachesExternalChildrenOnly )
{
    FWorld World( MakeDesc( 99113ULL ) );
    const TSharedRef<AActor> Parent = SpawnProbe( World );
    const TSharedRef<AActor> Child  = SpawnProbe( World );
    RegisterPublishedActor( World, Parent );
    RegisterPublishedActor( World, Child );

    const FWorldCellId ParentCell{ 13ULL };
    const FWorldCellId ChildCell{ 14ULL };
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Parent->GetId(), ParentCell, FPersistentActorId{ 702ULL } ) );
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Child->GetId(), ChildCell, FPersistentActorId{ 703ULL } ) );
    LUMEN_CHECK_HAS_VALUE( World.TryHierarchyAttach( Child->GetId(), Parent->GetId() ) );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( ParentCell ) );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( ChildCell ) );
    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    LUMEN_CHECK_EQ( World.GetHierarchy().GetPublishedTransforms().size(), 2ULL );
    LUMEN_CHECK_TRUE( World.GetHierarchy().GetParentId( Child->GetId() ).HasValue() );

    LUMEN_CHECK_HAS_VALUE( World.TryPartitionUnloadCell( ParentCell ) );
    const TOptional<ActorID> DetachedParent = World.GetHierarchy().GetParentId( Child->GetId() );
    LUMEN_CHECK_EQ( DetachedParent.ValueOr( InvalidActorID ), InvalidActorID );
    LUMEN_CHECK_FALSE( World.IsActorDormant( Child->GetId() ) );
    LUMEN_CHECK_TRUE( World.IsActorDormant( Parent->GetId() ) );
}

LUMEN_INTEGRATION_TEST( FWorldPartitionIsolation, TwoWorldsKeepIndependentCellStates )
{
    FWorld First( MakeDesc( 99114ULL ) );
    FWorld Second( MakeDesc( 99115ULL ) );
    const TSharedRef<AActor> FirstActor  = SpawnProbe( First );
    const TSharedRef<AActor> SecondActor = SpawnProbe( Second );
    const FWorldCellId FirstCell{ 15ULL };
    const FWorldCellId SecondCell{ 15ULL };

    LUMEN_CHECK_HAS_VALUE( First.TryAssignActorToCell( FirstActor->GetId(), FirstCell, FPersistentActorId{ 704ULL } ) );
    LUMEN_CHECK_HAS_VALUE( Second.TryAssignActorToCell( SecondActor->GetId(), SecondCell, FPersistentActorId{ 705ULL } ) );
    LUMEN_CHECK_HAS_VALUE( First.TryPartitionLoadCell( FirstCell ) );

    LUMEN_CHECK_FALSE( First.IsActorDormant( FirstActor->GetId() ) );
    LUMEN_CHECK_TRUE( Second.IsActorDormant( SecondActor->GetId() ) );
    LUMEN_CHECK_EQ( Second.GetPartition()->FindCellState( SecondCell ).ValueOr( EWorldCellState::Loaded ),
                    EWorldCellState::Unloaded );
    LUMEN_CHECK_EQ( First.GetPartition()->FindCellState( FirstCell ).ValueOr( EWorldCellState::Unloaded ),
                    EWorldCellState::Loaded );
    LUMEN_CHECK_EQ( First.GetWorldId(), 99114ULL );
    LUMEN_CHECK_EQ( Second.GetWorldId(), 99115ULL );
}

LUMEN_INTEGRATION_TEST( FWorldPartitionReplication, DormantActorHasNoReplicationTombstone )
{
    FWorldDesc Desc = MakeDesc( 99116ULL );
    FReplicationConfig ReplicationConfig;
    ReplicationConfig.MaxActors          = 4ULL;
    ReplicationConfig.MaxRecordsPerFrame = ReplicationConfig.MaxActors * MaxReplicationRecordsPerActor;
    ReplicationConfig.MaxPayloadBytes    = ( ReplicationConfig.MaxRecordsPerFrame * MaxReplicationRecordBytes ) +
                                           ReplicationFrameHeaderBytes + ReplicationFrameChecksumBytes;
    Desc.ReplicationConfig               = ReplicationConfig;
    Desc.ReplicationRole                 = EActorRole::Authority;

    FWorld World( Desc );
    const TSharedRef<AActor> Active  = SpawnProbe( World );
    const TSharedRef<AActor> Dormant = SpawnProbe( World );
    FCaptureTransport Transport;
    FNoopReplicationHandler Handler;
    LUMEN_CHECK_HAS_VALUE( World.InitializeReplication( Transport, Handler ) );

    const FWorldCellId ActiveCell{ 20ULL };
    const FWorldCellId DormantCell{ 21ULL };
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Active->GetId(), ActiveCell, InvalidPersistentActorId ) );
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Dormant->GetId(), DormantCell, InvalidPersistentActorId ) );
    const TOptional<FPersistentActorId> ActiveLabel  = World.GetReplicationSystem()->FindPersistentId( Active->GetId() );
    const TOptional<FPersistentActorId> DormantLabel = World.GetReplicationSystem()->FindPersistentId( Dormant->GetId() );
    LUMEN_CHECK_TRUE( ActiveLabel.HasValue() );
    LUMEN_CHECK_TRUE( DormantLabel.HasValue() );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( ActiveCell ) );
    LUMEN_CHECK_TRUE( World.IsActorDormant( Dormant->GetId() ) );

    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    LUMEN_CHECK_TRUE( Transport.Captured.size() > 0ULL );

    FReplicationFrameBuilder Builder;
    Builder.Reserve( ReplicationConfig.MaxPayloadBytes, ReplicationConfig.MaxRecordsPerFrame );
    const auto Parsed = Builder.TryParse( TSpan<const Byte>( Transport.Captured.data(), Transport.Captured.size() ) );
    LUMEN_CHECK_HAS_VALUE( Parsed );
    Bool bFoundActive      = false;
    Bool bFoundDormant     = false;
    Bool bDormantDestroyed = false;
    for ( const FReplicationRecord &Record : ( *Parsed ).Records )
    {
        if ( Record.Actor == *ActiveLabel )
        {
            bFoundActive = true;
        }
        if ( Record.Actor == *DormantLabel )
        {
            bFoundDormant = true;
            if ( Record.Kind == EActorReplicationKind::Destroy )
            {
                bDormantDestroyed = true;
            }
        }
    }
    /** INFO: A dormant actor stays in the replicated view so an acked replica never tombstones its bound ID. */
    LUMEN_CHECK_TRUE( bFoundActive );
    LUMEN_CHECK_TRUE( bFoundDormant );
    LUMEN_CHECK_FALSE( bDormantDestroyed );
}

LUMEN_INTEGRATION_TEST( FWorldPartitionReplication, ConflictingAuthorityLabelHasNoSideEffect )
{
    FWorldDesc Desc = MakeDesc( 99119ULL );
    FReplicationConfig ReplicationConfig;
    ReplicationConfig.MaxActors          = 4ULL;
    ReplicationConfig.MaxRecordsPerFrame = ReplicationConfig.MaxActors * MaxReplicationRecordsPerActor;
    ReplicationConfig.MaxPayloadBytes    = ( ReplicationConfig.MaxRecordsPerFrame * MaxReplicationRecordBytes ) +
                                           ReplicationFrameHeaderBytes + ReplicationFrameChecksumBytes;
    Desc.ReplicationConfig               = ReplicationConfig;
    Desc.ReplicationRole                 = EActorRole::Authority;

    FWorld World( Desc );
    const TSharedRef<AActor> First  = SpawnProbe( World );
    const TSharedRef<AActor> Second = SpawnProbe( World );
    FCaptureTransport Transport;
    FNoopReplicationHandler Handler;
    LUMEN_CHECK_HAS_VALUE( World.InitializeReplication( Transport, Handler ) );

    const FWorldCellId FirstCell{ 22ULL };
    const FWorldCellId SecondCell{ 23ULL };
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( First->GetId(), FirstCell, InvalidPersistentActorId ) );
    const TOptional<FPersistentActorId> FirstLabel = World.GetReplicationSystem()->FindPersistentId( First->GetId() );
    LUMEN_CHECK_TRUE( FirstLabel.HasValue() );

    LUMEN_CHECK_HAS_ERROR( World.TryAssignActorToCell( Second->GetId(), SecondCell, *FirstLabel ) );
    LUMEN_CHECK_FALSE( World.GetReplicationSystem()->FindPersistentId( Second->GetId() ).HasValue() );
    LUMEN_CHECK_FALSE( World.GetPartition()->FindActorCell( Second->GetId() ).HasValue() );

    /** INFO: A fresh label is allocated by the authority, never supplied by the caller. */
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Second->GetId(), SecondCell, InvalidPersistentActorId ) );
    const TOptional<FPersistentActorId> SecondLabel = World.GetReplicationSystem()->FindPersistentId( Second->GetId() );
    LUMEN_CHECK_TRUE( SecondLabel.HasValue() );
    LUMEN_CHECK_NOT_EQ( *SecondLabel, *FirstLabel );
}

LUMEN_INTEGRATION_TEST( FWorldPartitionLifecycle, UnassignedActorCanBeDestroyed )
{
    FWorld World( MakeDesc( 99120ULL ) );
    LUMEN_CHECK_TRUE( World.IsPartitionEnabled() );
    const TSharedRef<AActor> Member = SpawnProbe( World );
    LUMEN_CHECK_FALSE( World.GetPartition()->FindActorCell( Member->GetId() ).HasValue() );
    LUMEN_CHECK_HAS_VALUE( World.TryDestroyActor( Member->GetId() ) );
}

LUMEN_INTEGRATION_TEST( FWorldPartitionLifecycle, DestroyRefusedWhileJournalFull )
{
    FWorldDesc Desc                        = MakeDesc( 99121ULL );
    Desc.PartitionConfig.MaxJournalRecords = 1ULL;
    FWorld World( Desc );
    const TSharedRef<AActor> Member = SpawnProbe( World );
    const FWorldCellId Cell{ 40ULL };
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Member->GetId(), Cell, FPersistentActorId{ 720ULL } ) );
    LUMEN_CHECK_EQ( World.GetPartition()->GetStats().JournalRecords, 1ULL );
    LUMEN_CHECK_HAS_ERROR( World.TryDestroyActor( Member->GetId() ) );
    LUMEN_CHECK_TRUE( World.GetPartition()->FindActorCell( Member->GetId() ).HasValue() );
}

} // namespace Lumen
