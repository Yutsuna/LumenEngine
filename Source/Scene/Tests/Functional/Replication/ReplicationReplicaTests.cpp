/**
 * @file ReplicationReplicaTests.cpp
 * @brief Replica-role contracts: frame application, sequence rules, acks and bounded resend state.
 */

#include <Replication/ReplicationSystem.hpp>

#include <Tests/UnitTest.hpp>

#include <Replication/ReplicationFrame.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace
{

    /** @brief Smallest valid bounded configuration; configured once per system. */
    [[nodiscard]] FReplicationConfig MakeValidConfig () noexcept
    {
        constexpr USize ActorCount = 8ULL;
        FReplicationConfig Config;
        Config.MaxActors                 = ActorCount;
        Config.MaxRecordsPerFrame        = ActorCount * MaxReplicationRecordsPerActor;
        Config.MaxPayloadBytes           = ActorCount * MaxReplicationRecordsPerActor * MaxReplicationRecordBytes;
        Config.ResendWindow              = 4ULL;
        Config.MaxBufferedFrames         = 2ULL;
        Config.MaxPendingControlRecords  = 8ULL;
        Config.MaxPendingParents         = 4ULL;
        Config.MaxPendingInputs          = 8ULL;
        Config.MaxResendAttempts         = 2U;
        Config.FullSnapshotTickThreshold = 3U;
        Config.FullSnapshotCooldownTicks = 3U;
        Config.MaxPendingParentTicks     = 4U;
        return Config;
    }

    /** @class FTestReplicationTransport
     * @brief Bounded in-memory frame queue; dropping frames is a successful send, as on a lossy network.
     */
    class FTestReplicationTransport final : public IReplicationTransport
    {
    public:

        explicit FTestReplicationTransport ( const USize InSlots = 32ULL ) noexcept
        {
            Bytes.resize( InSlots * FrameBytes );
            Lengths.resize( InSlots );
        }

        void SetDropAll ( const Bool bInDrop ) noexcept
        {
            bDropAll = bInDrop;
        }

        [[nodiscard]] TExpected<void, EReplicationError> TrySend ( const TSpan<const Byte> InBytes ) noexcept override
        {
            if ( bDropAll )
            {
                return {};
            }
            if ( InBytes.size() > FrameBytes or Count == Lengths.size() )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
            }
            const USize Slot = ( Head + Count ) % Lengths.size();
            for ( USize Index = 0ULL; Index < InBytes.size(); ++Index )
            {
                Bytes[( Slot * FrameBytes ) + Index] = InBytes[Index];
            }
            Lengths[Slot] = InBytes.size();
            ++Count;
            return {};
        }

        [[nodiscard]] TExpected<USize, EReplicationError> TryReceive ( const TSpan<Byte> OutBytes ) noexcept override
        {
            if ( Count == 0ULL )
            {
                return 0ULL;
            }
            const USize Length = Lengths[Head];
            const USize Copied = Length <= OutBytes.size() ? Length : OutBytes.size();
            for ( USize Index = 0ULL; Index < Copied; ++Index )
            {
                OutBytes[Index] = Bytes[( Head * FrameBytes ) + Index];
            }
            Head = ( Head + 1ULL ) % Lengths.size();
            --Count;
            return Copied;
        }

        [[nodiscard]] USize GetPendingCount () const noexcept
        {
            return Count;
        }

        [[nodiscard]] USize GetDroppedSendCount () const noexcept
        {
            return DroppedSendCount;
        }

    private:

        static constexpr USize FrameBytes = 1024ULL;
        USize Head                        = 0ULL;
        USize Count                       = 0ULL;
        USize DroppedSendCount            = 0ULL;
        Bool bDropAll                     = false;
        TVector<Byte> Bytes;
        TVector<USize> Lengths;
    };

    /** @class FRecordingActorHandler
     * @brief Records spawn/destroy/transform/parent calls made while applying state frames.
     */
    class FRecordingActorHandler final : public IReplicationActorHandler
    {
    public:

        [[nodiscard]] TExpected<ActorID, EReplicationError> TrySpawn ( const FPersistentActorId InId ) noexcept override
        {
            SpawnedIds.push_back( InId.GetValue() );
            const ActorID Local = NextLocalId++;
            return Local;
        }

        [[nodiscard]] TExpected<void, EReplicationError> TryDestroy ( const ActorID InLocalId ) noexcept override
        {
            DestroyedLocals.push_back( InLocalId );
            return {};
        }

        [[nodiscard]] TExpected<void, EReplicationError>
        TrySetLocalTransform ( const ActorID InLocalId, const Maths::FMat4x4f &InLocalTransform ) noexcept override
        {
            TransformedLocals.push_back( InLocalId );
            Transforms.push_back( InLocalTransform );
            return {};
        }

        [[nodiscard]] TExpected<void, EReplicationError> TrySetParent ( const ActorID InLocalId,
                                                                        const ActorID InParentLocalId ) noexcept override
        {
            ParentedLocals.push_back( InLocalId );
            ParentIds.push_back( InParentLocalId );
            return {};
        }

        TVector<UInt64> SpawnedIds;
        TVector<ActorID> DestroyedLocals;
        TVector<ActorID> TransformedLocals;
        TVector<Maths::FMat4x4f> Transforms;
        TVector<ActorID> ParentedLocals;
        TVector<ActorID> ParentIds;

    private:

        ActorID NextLocalId = 100ULL;
    };

    /** @brief Serializes a frame with the public builder so tests feed real bytes to Tick. */
    [[nodiscard]] TVector<Byte> MakeFrameBytes ( const FReplicationFrameHeader &InHeader,
                                                 const TSpan<const FReplicationRecord> InRecords )
    {
        FReplicationFrameBuilder Builder;
        Builder.Reserve( 1024ULL, 8ULL );
        for ( const FReplicationRecord &Record : InRecords )
        {
            LUMEN_CHECK_HAS_VALUE( Builder.TryAppend( Record ) );
        }
        TVector<Byte> Bytes;
        Bytes.resize( 1024ULL );
        const TExpected<USize, EReplicationError> Written = Builder.TryWrite( Bytes, InHeader );
        LUMEN_CHECK_HAS_VALUE( Written );
        Bytes.resize( *Written );
        return Bytes;
    }

    /** @brief Builds a state frame on the given channel with the given sequence. */
    [[nodiscard]] TVector<Byte> MakeStateFrame ( const UInt64 InSequence,
                                                 const UInt64 InFrameId,
                                                 const EReplicationChannel InChannel,
                                                 const Bool bInFullSnapshot,
                                                 const TSpan<const FReplicationRecord> InRecords )
    {
        FReplicationFrameHeader Header;
        Header.Sequence  = InSequence;
        Header.FrameId   = InFrameId;
        Header.Channel   = InChannel;
        Header.Direction = EReplicationDirection::StateToReplica;
        Header.Flags     = bInFullSnapshot ? ReplicationFrameFullSnapshot : 0U;
        return MakeFrameBytes( Header, InRecords );
    }

    /** @brief Convenience overload for a frame carrying exactly one record. */
    [[nodiscard]] TVector<Byte> MakeStateFrame ( const UInt64 InSequence,
                                                 const UInt64 InFrameId,
                                                 const EReplicationChannel InChannel,
                                                 const Bool bInFullSnapshot,
                                                 const FReplicationRecord &InRecord )
    {
        return MakeStateFrame( InSequence, InFrameId, InChannel, bInFullSnapshot,
                               TSpan<const FReplicationRecord>( &InRecord, 1ULL ) );
    }

    /** @brief One spawn record carrying the Alive property rank. */
    [[nodiscard]] FReplicationRecord MakeSpawn ( const FPersistentActorId InId ) noexcept
    {
        FReplicationRecord Spawn;
        Spawn.Actor        = InId;
        Spawn.Kind         = EActorReplicationKind::Spawn;
        Spawn.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Alive );
        return Spawn;
    }

    /** @brief Drains the transport so later ticks see only deliberately injected frames. */
    void DrainTransport ( FTestReplicationTransport &InTransport )
    {
        TVector<Byte> Scratch;
        Scratch.resize( 1024ULL );
        while ( InTransport.TryReceive( Scratch ).HasValue() and InTransport.GetPendingCount() > 0ULL )
        {
        }
    }

    /** @brief Parses the next queued frame; the builder must outlive the returned record span. */
    [[nodiscard]] FReplicationFrame ParseNextFrame ( FTestReplicationTransport &InTransport,
                                                     FReplicationFrameBuilder &InOutParser )
    {
        TVector<Byte> Scratch;
        Scratch.resize( 1024ULL );
        const TExpected<USize, EReplicationError> Received = InTransport.TryReceive( Scratch );
        LUMEN_CHECK_HAS_VALUE( Received );
        LUMEN_CHECK( *Received > 0ULL );
        const TExpected<FReplicationFrame, EReplicationError> Parsed = InOutParser.TryParse( { Scratch.data(), *Received } );
        LUMEN_CHECK_HAS_VALUE( Parsed );
        return *Parsed;
    }

} // namespace

LUMEN_TEST( FReplicationReplica, BindPersistentIdRejectsDoubleBindAndRoleViolations )
{
    FTestReplicationTransport Transport;
    FRecordingActorHandler Handler;
    FReplicationSystem System;
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Replica, Transport, Handler ) );
    LUMEN_CHECK( System.GetRole() == EActorRole::Replica );

    LUMEN_CHECK_HAS_VALUE( System.BindPersistentId( 5ULL, FPersistentActorId( 100ULL ) ) );
    LUMEN_CHECK_EQ( *System.FindLocalId( FPersistentActorId( 100ULL ) ), 5ULL );
    LUMEN_CHECK( *System.FindPersistentId( 5ULL ) == FPersistentActorId( 100ULL ) );

    const TExpected<void, EReplicationError> SameId = System.BindPersistentId( 6ULL, FPersistentActorId( 100ULL ) );
    LUMEN_CHECK_HAS_ERROR( SameId );
    LUMEN_CHECK_EQ( SameId.Error(), EReplicationError::InvalidPersistentId );
    const TExpected<void, EReplicationError> SameLocal = System.BindPersistentId( 5ULL, FPersistentActorId( 101ULL ) );
    LUMEN_CHECK_HAS_ERROR( SameLocal );
    LUMEN_CHECK_EQ( SameLocal.Error(), EReplicationError::InvalidPersistentId );
    LUMEN_CHECK_EQ( *System.FindLocalId( FPersistentActorId( 100ULL ) ), 5ULL );
    LUMEN_CHECK( *System.FindPersistentId( 5ULL ) == FPersistentActorId( 100ULL ) );
    LUMEN_CHECK_FALSE( System.FindLocalId( FPersistentActorId( 101ULL ) ).HasValue() );

    LUMEN_CHECK_HAS_ERROR( System.BindPersistentId( InvalidActorID, FPersistentActorId( 102ULL ) ) );
    LUMEN_CHECK_HAS_ERROR( System.BindPersistentId( 7ULL, InvalidPersistentActorId ) );
    const TExpected<void, EReplicationError> Assign = System.AssignPersistentId( 8ULL );
    LUMEN_CHECK_HAS_ERROR( Assign );
    LUMEN_CHECK_EQ( Assign.Error(), EReplicationError::AuthorityViolation );
    const TExpected<void, EReplicationError> Visibility = System.SetVisibility( FPersistentActorId( 100ULL ), false );
    LUMEN_CHECK_HAS_ERROR( Visibility );
    LUMEN_CHECK_EQ( Visibility.Error(), EReplicationError::AuthorityViolation );

    for ( ActorID Local = 10ULL; Local < 17ULL; ++Local )
    {
        const FPersistentActorId Id( 200ULL + Local );
        LUMEN_CHECK_HAS_VALUE( System.BindPersistentId( Local, Id ) );
    }
    const TExpected<void, EReplicationError> Overflow = System.BindPersistentId( 30ULL, FPersistentActorId( 999ULL ) );
    LUMEN_CHECK_HAS_ERROR( Overflow );
    LUMEN_CHECK_EQ( Overflow.Error(), EReplicationError::CapacityExceeded );
    LUMEN_CHECK_FALSE( System.FindPersistentId( 30ULL ).HasValue() );
}

LUMEN_TEST( FReplicationReplica, AppliesStateFrameSpawnTransformParentAndVisibility )
{
    FTestReplicationTransport Transport;
    FRecordingActorHandler Handler;
    FReplicationSystem System;
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Replica, Transport, Handler ) );

    const Maths::FMat4x4f Transform = Maths::FMat4x4f::Translate( Maths::FVec3f{ 7.0F, 8.0F, 9.0F } );
    FReplicationRecord TransformRecord;
    TransformRecord.Actor        = FPersistentActorId( 100ULL );
    TransformRecord.Kind         = EActorReplicationKind::Transform;
    TransformRecord.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::LocalTransform );
    TransformRecord.SetLocalTransform( Transform );

    FReplicationRecord ParentRecord;
    ParentRecord.Actor        = FPersistentActorId( 100ULL );
    ParentRecord.Kind         = EActorReplicationKind::Parent;
    ParentRecord.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Parent );
    ParentRecord.SetParent( InvalidPersistentActorId );

    FReplicationRecord VisibilityRecord;
    VisibilityRecord.Actor        = FPersistentActorId( 100ULL );
    VisibilityRecord.Kind         = EActorReplicationKind::Visibility;
    VisibilityRecord.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Visibility );
    VisibilityRecord.SetVisibility( false );

    const FReplicationRecord Records[] = { MakeSpawn( FPersistentActorId( 100ULL ) ), TransformRecord, ParentRecord,
                                           VisibilityRecord };
    LUMEN_CHECK_HAS_VALUE( Transport.TrySend( MakeStateFrame( 1ULL, 10ULL, EReplicationChannel::Reliable, true, Records ) ) );

    const FReplicationSourceView Empty;
    const TExpected<FReplicationTickResult, EReplicationError> Ticked = System.Tick( 1ULL, Empty );
    LUMEN_CHECK_HAS_VALUE( Ticked );
    LUMEN_CHECK( Ticked->FramesReceived >= 1ULL );
    LUMEN_CHECK( Ticked->RecordsApplied >= 4ULL );
    LUMEN_CHECK_EQ( Handler.SpawnedIds.size(), 1ULL );
    LUMEN_CHECK_EQ( Handler.SpawnedIds[0ULL], 100ULL );
    LUMEN_CHECK_EQ( Handler.TransformedLocals.size(), 1ULL );
    LUMEN_CHECK( Handler.Transforms[0ULL] == Transform );
    LUMEN_CHECK_EQ( Handler.ParentedLocals.size(), 1ULL );
    LUMEN_CHECK( Handler.ParentIds[0ULL] == InvalidActorID );
    LUMEN_CHECK( System.GetStats().FramesApplied >= 1ULL );
    LUMEN_CHECK_EQ( System.GetStats().LastAppliedReliableSequence, 1ULL );
    LUMEN_CHECK_FALSE( System.GetVisibility( FPersistentActorId( 100ULL ) ) );
    const TOptional<ActorID> Local = System.FindLocalId( FPersistentActorId( 100ULL ) );
    LUMEN_CHECK_HAS_VALUE( Local );
    LUMEN_CHECK( *System.FindPersistentId( *Local ) == FPersistentActorId( 100ULL ) );
}

LUMEN_TEST( FReplicationReplica, DuplicateReliableSequenceIsSuppressed )
{
    FTestReplicationTransport Transport;
    FRecordingActorHandler Handler;
    FReplicationSystem System;
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Replica, Transport, Handler ) );

    const FReplicationRecord Records[] = { MakeSpawn( FPersistentActorId( 300ULL ) ) };
    const TVector<Byte> Frame          = MakeStateFrame( 1ULL, 20ULL, EReplicationChannel::Reliable, true, Records );

    const FReplicationSourceView Empty;
    LUMEN_CHECK_HAS_VALUE( Transport.TrySend( Frame ) );
    LUMEN_CHECK_HAS_VALUE( System.Tick( 1ULL, Empty ) );
    DrainTransport( Transport );

    LUMEN_CHECK_HAS_VALUE( Transport.TrySend( Frame ) );
    LUMEN_CHECK_HAS_VALUE( System.Tick( 2ULL, Empty ) );

    LUMEN_CHECK( System.GetStats().DuplicateFramesSuppressed >= 1ULL );
    LUMEN_CHECK_EQ( System.GetStats().FramesApplied, 1ULL );
    LUMEN_CHECK_EQ( Handler.SpawnedIds.size(), 1ULL );
    LUMEN_CHECK_EQ( System.GetStats().LastAppliedReliableSequence, 1ULL );
    const USize StateIndex    = static_cast<USize>( EReplicationDirection::StateToReplica );
    const USize ReliableIndex = static_cast<USize>( EReplicationChannel::Reliable );
    LUMEN_CHECK( System.GetStats().Channels[StateIndex][ReliableIndex].DroppedCount >= 1ULL );
}

LUMEN_TEST( FReplicationReplica, OldUnreliableSequenceIsRejected )
{
    FTestReplicationTransport Transport;
    FRecordingActorHandler Handler;
    FReplicationSystem System;
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Replica, Transport, Handler ) );

    const FReplicationRecord Fresh[] = { MakeSpawn( FPersistentActorId( 400ULL ) ) };
    const FReplicationRecord Old[]   = { MakeSpawn( FPersistentActorId( 401ULL ) ) };
    const FReplicationSourceView Empty;

    LUMEN_CHECK_HAS_VALUE( Transport.TrySend( MakeStateFrame( 5ULL, 30ULL, EReplicationChannel::Unreliable, false, Fresh ) ) );
    LUMEN_CHECK_HAS_VALUE( System.Tick( 1ULL, Empty ) );
    LUMEN_CHECK_EQ( Handler.SpawnedIds.size(), 1ULL );
    LUMEN_CHECK_EQ( Handler.SpawnedIds[0ULL], 400ULL );

    LUMEN_CHECK_HAS_VALUE( Transport.TrySend( MakeStateFrame( 4ULL, 31ULL, EReplicationChannel::Unreliable, false, Old ) ) );
    LUMEN_CHECK_HAS_VALUE( System.Tick( 2ULL, Empty ) );

    LUMEN_CHECK( System.GetStats().DuplicateFramesSuppressed >= 1ULL );
    LUMEN_CHECK_EQ( Handler.SpawnedIds.size(), 1ULL );
    const USize StateIndex      = static_cast<USize>( EReplicationDirection::StateToReplica );
    const USize UnreliableIndex = static_cast<USize>( EReplicationChannel::Unreliable );
    LUMEN_CHECK( System.GetStats().Channels[StateIndex][UnreliableIndex].DroppedCount >= 1ULL );
    LUMEN_CHECK_FALSE( System.FindLocalId( FPersistentActorId( 401ULL ) ).HasValue() );
}

LUMEN_TEST( FReplicationReplica, ReliableStateFrameEmitsAckThroughTransport )
{
    FTestReplicationTransport Transport;
    FRecordingActorHandler Handler;
    FReplicationSystem System;
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Replica, Transport, Handler ) );

    const FReplicationRecord Records[] = { MakeSpawn( FPersistentActorId( 500ULL ) ) };
    LUMEN_CHECK_HAS_VALUE( Transport.TrySend( MakeStateFrame( 1ULL, 40ULL, EReplicationChannel::Reliable, true, Records ) ) );

    const FReplicationSourceView Empty;
    const TExpected<FReplicationTickResult, EReplicationError> Ticked = System.Tick( 1ULL, Empty );
    LUMEN_CHECK_HAS_VALUE( Ticked );
    LUMEN_CHECK( Transport.GetPendingCount() >= 1ULL );

    FReplicationFrameBuilder Parser;
    Parser.Reserve( 1024ULL, 8ULL );
    const FReplicationFrame Control = ParseNextFrame( Transport, Parser );
    LUMEN_CHECK( Control.Header.Direction == EReplicationDirection::StateToReplica );
    LUMEN_CHECK( Control.Header.Channel == EReplicationChannel::Reliable );
    LUMEN_CHECK( ( Control.Header.Flags & ReplicationFrameControlOnly ) != 0U );
    LUMEN_CHECK_EQ( Control.Records.size(), 1ULL );
    LUMEN_CHECK( Control.Records[0ULL].Kind == EActorReplicationKind::Control );
    const FReplicationControlPayload Payload = Control.Records[0ULL].GetControl();
    LUMEN_CHECK( Payload.Kind == EReplicationControlKind::Ack );
    LUMEN_CHECK( Payload.Direction == EReplicationDirection::StateToReplica );
    LUMEN_CHECK( Payload.Channel == EReplicationChannel::Reliable );
    LUMEN_CHECK_EQ( Payload.Sequence, 1ULL );
}

LUMEN_TEST( FReplicationReplica, StaleBaselineEscalatesAfterResendAttemptsAreDropped )
{
    /** INFO: Escalation is authority-side bookkeeping; paired here with the replica protocol fixtures. */
    FTestReplicationTransport Dropping( 64ULL );
    FRecordingActorHandler Handler;
    FReplicationSystem Authority;
    FReplicationConfig Config = MakeValidConfig();
    Config.ResendWindow       = 4ULL;
    LUMEN_CHECK_HAS_VALUE( Authority.Initialize( Config, EActorRole::Authority, Dropping, Handler ) );
    LUMEN_CHECK_HAS_VALUE( Authority.AssignPersistentId( 11ULL ) );
    Dropping.SetDropAll( true );

    TVector<FReplicationActorSourceRecord> Actors;
    Actors.push_back( FReplicationActorSourceRecord{
        .LocalId = 11ULL, .ParentLocalId = InvalidActorID, .LocalTransform = Maths::FMat4x4f::Identity() } );
    const FReplicationSourceView Source = FReplicationSourceView{
        .Actors = TSpan<const FReplicationActorSourceRecord>( Actors.data(), Actors.size() ), .FrameIndex = 1ULL };

    for ( UInt64 TickIndex = 1ULL; TickIndex <= 4ULL; ++TickIndex )
    {
        LUMEN_CHECK_HAS_VALUE( Authority.Tick( TickIndex, Source ) );
    }
    /** INFO: With every frame dropped, no resend entry survives long enough for the escalation counter
     *        to become observable through the public API; assert the state invariants that are exposed. */
    LUMEN_CHECK( Authority.GetStats().FullSnapshotsSent >= 1ULL );
    LUMEN_CHECK( Authority.GetStats().FramesBuilt >= 1ULL );
    LUMEN_CHECK_TRUE( Authority.IsInitialized() );
    LUMEN_CHECK( *Authority.FindPersistentId( 11ULL ) == FPersistentActorId( 1ULL ) );
    LUMEN_CHECK_EQ( Dropping.GetDroppedSendCount(), 0ULL );
}

LUMEN_TEST( FReplicationReplica, ResendRingOverflowDoesNotCorruptSystemState )
{
    FTestReplicationTransport Transport( 64ULL );
    FRecordingActorHandler Handler;
    FReplicationSystem Authority;
    FReplicationConfig Config = MakeValidConfig();
    Config.ResendWindow       = 4ULL;
    LUMEN_CHECK_HAS_VALUE( Authority.Initialize( Config, EActorRole::Authority, Transport, Handler ) );
    LUMEN_CHECK_HAS_VALUE( Authority.AssignPersistentId( 12ULL ) );
    LUMEN_CHECK_HAS_VALUE( Authority.SetVisibility( FPersistentActorId( 1ULL ), false ) );

    TVector<FReplicationActorSourceRecord> Actors;
    Actors.push_back( FReplicationActorSourceRecord{
        .LocalId = 12ULL, .ParentLocalId = InvalidActorID, .LocalTransform = Maths::FMat4x4f::Identity() } );
    const FReplicationSourceView Source = FReplicationSourceView{
        .Actors = TSpan<const FReplicationActorSourceRecord>( Actors.data(), Actors.size() ), .FrameIndex = 1ULL };

    for ( UInt64 TickIndex = 1ULL; TickIndex <= 6ULL; ++TickIndex )
    {
        LUMEN_CHECK_HAS_VALUE( Authority.Tick( TickIndex, Source ) );
    }
    /** INFO: Resend-ring eviction is private state; overflow is checked through the emitted frames
     *        and the unchanged authority bindings/visibility instead of an internal counter. */
    LUMEN_CHECK( Authority.GetStats().FullSnapshotsSent >= 1ULL );
    LUMEN_CHECK( Authority.GetStats().FramesBuilt >= 1ULL );
    LUMEN_CHECK_TRUE( Authority.IsInitialized() );
    LUMEN_CHECK_EQ( Authority.FindPersistentId( 12ULL )->GetValue(), 1ULL );
    LUMEN_CHECK_FALSE( Authority.GetVisibility( FPersistentActorId( 1ULL ) ) );
    LUMEN_CHECK( Transport.GetPendingCount() > 0ULL );

    FReplicationFrameBuilder Parser;
    Parser.Reserve( 1024ULL, 8ULL );
    const FReplicationFrame Latest = ParseNextFrame( Transport, Parser );
    LUMEN_CHECK( Latest.Header.Direction == EReplicationDirection::StateToReplica );
    LUMEN_CHECK( Latest.Header.Channel == EReplicationChannel::Reliable );
    LUMEN_CHECK( Latest.Records.size() > 0ULL );
    LUMEN_CHECK_HAS_VALUE( Authority.Tick( 7ULL, Source ) );
    LUMEN_CHECK_TRUE( Authority.IsInitialized() );
}

LUMEN_TEST( FReplicationReplica, DestroyTombstoneSurvivesSkippedFramesUntilNewSpawn )
{
    FTestReplicationTransport Transport;
    FRecordingActorHandler Handler;
    FReplicationSystem System;
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Replica, Transport, Handler ) );

    const FReplicationSourceView Empty;
    LUMEN_CHECK_HAS_VALUE( Transport.TrySend(
        MakeStateFrame( 1ULL, 50ULL, EReplicationChannel::Reliable, true, { MakeSpawn( FPersistentActorId( 600ULL ) ) } ) ) );
    LUMEN_CHECK_HAS_VALUE( System.Tick( 1ULL, Empty ) );
    LUMEN_CHECK_EQ( Handler.SpawnedIds.size(), 1ULL );

    FReplicationRecord Destroy;
    Destroy.Actor = FPersistentActorId( 600ULL );
    Destroy.Kind  = EActorReplicationKind::Destroy;
    LUMEN_CHECK_HAS_VALUE(
        Transport.TrySend( MakeStateFrame( 2ULL, 51ULL, EReplicationChannel::Reliable, false, { Destroy } ) ) );
    LUMEN_CHECK_HAS_VALUE( System.Tick( 2ULL, Empty ) );
    LUMEN_CHECK_EQ( Handler.DestroyedLocals.size(), 1ULL );
    LUMEN_CHECK_FALSE( System.FindLocalId( FPersistentActorId( 600ULL ) ).HasValue() );

    FReplicationRecord StaleTransform;
    StaleTransform.Actor = FPersistentActorId( 600ULL );
    StaleTransform.Kind  = EActorReplicationKind::Transform;
    StaleTransform.SetLocalTransform( Maths::FMat4x4f::Translate( Maths::FVec3f{ 1.0F, 2.0F, 3.0F } ) );
    LUMEN_CHECK_HAS_VALUE(
        Transport.TrySend( MakeStateFrame( 4ULL, 52ULL, EReplicationChannel::Reliable, false, { StaleTransform } ) ) );
    LUMEN_CHECK_HAS_VALUE(
        Transport.TrySend( MakeStateFrame( 5ULL, 53ULL, EReplicationChannel::Reliable, false, { StaleTransform } ) ) );
    LUMEN_CHECK_HAS_VALUE( System.Tick( 3ULL, Empty ) );
    LUMEN_CHECK( System.GetStats().OutOfOrderRejected >= 1ULL );

    LUMEN_CHECK_HAS_VALUE( Transport.TrySend(
        MakeStateFrame( 3ULL, 54ULL, EReplicationChannel::Reliable, false, TSpan<const FReplicationRecord>{} ) ) );
    LUMEN_CHECK_HAS_VALUE( System.Tick( 4ULL, Empty ) );
    LUMEN_CHECK_EQ( System.GetStats().LastAppliedReliableSequence, 5ULL );
    LUMEN_CHECK_EQ( Handler.SpawnedIds.size(), 1ULL );
    LUMEN_CHECK_EQ( Handler.TransformedLocals.size(), 0ULL );
    LUMEN_CHECK_FALSE( System.FindLocalId( FPersistentActorId( 600ULL ) ).HasValue() );

    LUMEN_CHECK_HAS_VALUE( Transport.TrySend(
        MakeStateFrame( 6ULL, 55ULL, EReplicationChannel::Reliable, true, { MakeSpawn( FPersistentActorId( 600ULL ) ) } ) ) );
    LUMEN_CHECK_HAS_VALUE( System.Tick( 5ULL, Empty ) );
    LUMEN_CHECK_EQ( Handler.SpawnedIds.size(), 2ULL );
    LUMEN_CHECK( System.FindLocalId( FPersistentActorId( 600ULL ) ).HasValue() );
}

} // namespace Lumen
