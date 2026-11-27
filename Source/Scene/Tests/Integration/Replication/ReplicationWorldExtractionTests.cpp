/**
 * @file ReplicationWorldExtractionTests.cpp
 * @brief Replication visibility is published through the existing renderer DTO boundary.
 */

#include <World/World.hpp>

#include <Actor/Actor.hpp>
#include <Replication/ReplicationSystem.hpp>

#include <Rendering/RenderRecords.hpp>

#include <Tests/UnitTest.hpp>

#include "ReplicationTestSupport.hpp"

namespace Lumen
{
namespace
{

    constexpr USize ActorCount = 8ULL;

    class FReplicationWorldActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FReplicationWorldActor );

    public:

        explicit FReplicationWorldActor ( const ActorID InId ) noexcept : AActor( InId )
        {
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            static_cast<void>( InMessage );
        }

        void OnTick ( const Float64 InDeltaTime ) noexcept override
        {
            static_cast<void>( InDeltaTime );
        }
    };

    FWorldDesc MakeDesc () noexcept
    {
        using namespace ReplicationTests;
        FWorldDesc Desc;
        Desc.WorkerCount       = 1U;
        Desc.MaxActors         = ActorCount;
        Desc.HierarchyCapacity = ActorCount;
        Desc.WorldId           = 88011ULL;
        Desc.RenderCapacities  = FRenderSnapshotCapacities{ .TransformCapacity = ActorCount };
        Desc.ReplicationConfig = MakeConfig( ActorCount );
        Desc.ReplicationRole   = EActorRole::Authority;
        return Desc;
    }

} // namespace

LUMEN_INTEGRATION_TEST( FReplicationWorldExtraction, ReplicatedVisibilityReachesRenderSnapshot )
{
    using namespace ReplicationTests;
    FWorld World( MakeDesc() );
    FMemoryTransport StateTransport( MakeConfig( ActorCount ).MaxPayloadBytes );
    FRecordingHandler Handler;
    LUMEN_CHECK_HAS_VALUE( World.InitializeReplication( StateTransport, Handler ) );
    LUMEN_CHECK_TRUE( World.IsReplicationEnabled() );

    TSharedRef<FReplicationWorldActor> Actor = *World.TrySpawnActor<FReplicationWorldActor>();
    const ActorID LocalId                    = Actor->GetId();
    LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( LocalId ) );
    LUMEN_CHECK_HAS_VALUE( World.GetSpatialRegistry().TryRegisterSpatialEntity( LocalId ) );
    for ( UInt32 Frame = 0U; Frame < 4U; ++Frame )
    {
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    }

    FReplicationSystem *Replication = World.GetReplicationSystem();
    LUMEN_CHECK_TRUE( Replication != nullptr );
    const TOptional<FPersistentActorId> Persistent = Replication->FindPersistentId( LocalId );
    LUMEN_CHECK_HAS_VALUE( Persistent );
    LUMEN_CHECK_HAS_VALUE( Replication->SetVisibility( *Persistent, false ) );
    LUMEN_CHECK_FALSE( Replication->GetVisibility( *Persistent ) );
    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );

    LUMEN_CHECK_HAS_VALUE( World.PublishRenderSnapshot() );
    LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
    const FRenderSnapshotView View      = World.GetRenderSnapshot();
    const FRenderTransformRecord *Found = nullptr;
    for ( const FRenderTransformRecord &Record : View.Transforms )
    {
        if ( Record.ProxyId == FRenderProxyId{ LocalId } )
        {
            Found = &Record;
            break;
        }
    }
    LUMEN_CHECK_TRUE( Found != nullptr );
    LUMEN_CHECK_FALSE( ( Found->Flags & ERenderTransformFlags::Visible ) != ERenderTransformFlags::None );
}

LUMEN_INTEGRATION_TEST( FReplicationWorldExtraction, DisabledReplicationLeavesExtractionUnchanged )
{
    FWorldDesc Desc        = MakeDesc();
    Desc.ReplicationConfig = FReplicationConfig{};
    FWorld World( Desc );
    LUMEN_CHECK_TRUE( World.GetReplicationSystem() == nullptr );
    LUMEN_CHECK_FALSE( World.IsReplicationEnabled() );

    TSharedRef<FReplicationWorldActor> Actor = *World.TrySpawnActor<FReplicationWorldActor>();
    LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( Actor->GetId() ) );
    LUMEN_CHECK_HAS_VALUE( World.GetSpatialRegistry().TryRegisterSpatialEntity( Actor->GetId() ) );
    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    LUMEN_CHECK_HAS_VALUE( World.PublishRenderSnapshot() );
    LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
    const FRenderSnapshotView View = World.GetRenderSnapshot();
    LUMEN_CHECK_EQ( View.Transforms.size(), 1ULL );
    LUMEN_CHECK_TRUE( ( View.Transforms[0U].Flags & ERenderTransformFlags::Visible ) != ERenderTransformFlags::None );
}

} // namespace Lumen
