/**
 * @file ReplicationRoundTripTests.cpp
 * @brief End-to-end authority/replica state and lifecycle propagation.
 */

#include <Replication/ReplicationSystem.hpp>

#include <Tests/UnitTest.hpp>

#include "ReplicationTestSupport.hpp"

namespace Lumen
{
namespace
{

    constexpr USize ActorCount = 8ULL;

    void Pump ( FReplicationSystem &InAuthority,
                FReplicationSystem &InReplica,
                const FReplicationSourceView &InView,
                const UInt64 InFrameIndex ) noexcept
    {
        static_cast<void>( InAuthority.Tick( InFrameIndex, InView ) );
        static_cast<void>( InReplica.Tick( InFrameIndex, {} ) );
    }

} // namespace

LUMEN_INTEGRATION_TEST( FReplicationRoundTrip, AbsoluteStateAndVisibilityConverge )
{
    using namespace ReplicationTests;
    const FReplicationConfig Config = MakeConfig( ActorCount );
    FMemoryTransport StateTransport( Config.MaxPayloadBytes );
    FRecordingHandler AuthorityHandler;
    FRecordingHandler ReplicaHandler;
    FReplicationSystem Authority;
    FReplicationSystem Replica;
    LUMEN_CHECK_HAS_VALUE( Authority.Initialize( Config, EActorRole::Authority, StateTransport, AuthorityHandler ) );
    LUMEN_CHECK_HAS_VALUE( Replica.Initialize( Config, EActorRole::Replica, StateTransport, ReplicaHandler ) );

    TVector<FReplicationActorSourceRecord> Source;
    Source.reserve( ActorCount );
    Source.resize( ActorCount );
    for ( USize Index = 0ULL; Index < ActorCount; ++Index )
    {
        Source[Index].LocalId        = static_cast<ActorID>( Index + 1ULL );
        Source[Index].LocalTransform = Maths::FMat4x4f::Translate( Maths::FVec3f{ static_cast<Float32>( Index ), 0.0F, 0.0F } );
    }
    const FReplicationSourceView View{ .Actors = Source, .FrameIndex = 1ULL };

    for ( UInt64 Frame = 1ULL; Frame <= 8ULL; ++Frame )
    {
        Pump( Authority, Replica, View, Frame );
    }
    LUMEN_CHECK_EQ( ReplicaHandler.GetStates().size(), ActorCount );
    const UInt64 FirstDigest = ReplicaHandler.GetDigest();
    LUMEN_CHECK_NOT_EQ( FirstDigest, 0ULL );

    const auto Persistent = Authority.FindPersistentId( 1ULL );
    LUMEN_CHECK_HAS_VALUE( Persistent );
    LUMEN_CHECK_HAS_VALUE( Authority.SetVisibility( *Persistent, false ) );
    for ( UInt64 Frame = 9ULL; Frame <= 14ULL; ++Frame )
    {
        Pump( Authority, Replica, View, Frame );
    }
    /** INFO: Visibility is owned by the replication system, not by the actor handler digest. */
    LUMEN_CHECK_EQ( ReplicaHandler.GetDigest(), FirstDigest );
    LUMEN_CHECK_FALSE( Replica.GetVisibility( *Persistent ) );
}

LUMEN_INTEGRATION_TEST( FReplicationRoundTrip, DestroyTombstoneSurvivesSkippedFrames )
{
    using namespace ReplicationTests;
    const FReplicationConfig Config = MakeConfig( ActorCount );
    FMemoryTransport StateTransport( Config.MaxPayloadBytes );
    FRecordingHandler AuthorityHandler;
    FRecordingHandler ReplicaHandler;
    FReplicationSystem Authority;
    FReplicationSystem Replica;
    LUMEN_CHECK_HAS_VALUE( Authority.Initialize( Config, EActorRole::Authority, StateTransport, AuthorityHandler ) );
    LUMEN_CHECK_HAS_VALUE( Replica.Initialize( Config, EActorRole::Replica, StateTransport, ReplicaHandler ) );

    TVector<FReplicationActorSourceRecord> Source;
    Source.reserve( ActorCount );
    Source.resize( ActorCount );
    for ( USize Index = 0ULL; Index < ActorCount; ++Index )
    {
        Source[Index].LocalId = static_cast<ActorID>( Index + 1ULL );
    }
    const FReplicationSourceView FullView{ .Actors = Source, .FrameIndex = 1ULL };
    for ( UInt64 Frame = 1ULL; Frame <= 6ULL; ++Frame )
    {
        Pump( Authority, Replica, FullView, Frame );
    }
    LUMEN_CHECK_EQ( ReplicaHandler.GetStates().size(), ActorCount );

    /** INFO: Two actors leave the source view; the destroy records must survive three dropped frames. */
    Source.resize( ActorCount - 2ULL );
    const FReplicationSourceView ReducedView{ .Actors = Source, .FrameIndex = 7ULL };
    static_cast<void>( Authority.Tick( 7ULL, ReducedView ) );
    for ( UInt32 Dropped = 0U; Dropped < 3U; ++Dropped )
    {
        static_cast<void>( StateTransport.TryDropOldest() );
    }
    for ( UInt64 Frame = 8ULL; Frame <= 16ULL; ++Frame )
    {
        Pump( Authority, Replica, ReducedView, Frame );
    }
    LUMEN_CHECK_EQ( ReplicaHandler.GetStates().size(), ActorCount - 2ULL );
}

} // namespace Lumen
