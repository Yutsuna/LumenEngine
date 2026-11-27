/**
 * @file ReplicationAuthorityTests.cpp
 * @brief Authority-role contracts of FReplicationSystem over the public transport/handler boundaries.
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
     * @brief Bounded in-memory frame queue injected into the system under test.
     */
    class FTestReplicationTransport final : public IReplicationTransport
    {
    public:

        FTestReplicationTransport () noexcept
        {
            constexpr USize Slots = 16ULL;
            Bytes.resize( ( Slots * FrameBytes ) );
            Lengths.resize( Slots );
        }

        [[nodiscard]] TExpected<void, EReplicationError> TrySend ( const TSpan<const Byte> InBytes ) noexcept override
        {
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

    private:

        static constexpr USize FrameBytes = 1024ULL;
        USize Head                        = 0ULL;
        USize Count                       = 0ULL;
        TVector<Byte> Bytes;
        TVector<USize> Lengths;
    };

    /** @class FTestReplicationActorHandler
     * @brief Records every actor mutation requested through the replica-side injection boundary.
     */
    class FTestReplicationActorHandler final : public IReplicationActorHandler
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
            static_cast<void>( InLocalTransform );
            TransformedLocals.push_back( InLocalId );
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
        TVector<ActorID> ParentedLocals;
        TVector<ActorID> ParentIds;

    private:

        ActorID NextLocalId = 100ULL;
    };

    /** @brief Serializes a hand-crafted frame with the public builder. */
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

    /** @brief Drains all pending frames; the caller inspects the system stats afterwards. */
    void DrainTransport ( FTestReplicationTransport &InTransport )
    {
        TVector<Byte> Scratch;
        Scratch.resize( 1024ULL );
        while ( InTransport.TryReceive( Scratch ).HasValue() and InTransport.GetPendingCount() > 0ULL )
        {
        }
    }

    /** @brief One canonical source actor for authority ticks. */
    [[nodiscard]] FReplicationSourceView MakeSourceView ( const TVector<FReplicationActorSourceRecord> &InActors,
                                                          const UInt64 InFrameIndex ) noexcept
    {
        return FReplicationSourceView{ .Actors = TSpan<const FReplicationActorSourceRecord>( InActors.data(), InActors.size() ),
                                       .FrameIndex = InFrameIndex };
    }

} // namespace

LUMEN_TEST( FReplicationAuthority, InitializeAndPersistentIdAssignmentAreStable )
{
    FTestReplicationTransport Transport;
    FTestReplicationActorHandler Handler;
    FReplicationSystem System;

    LUMEN_CHECK_FALSE( System.IsInitialized() );
    LUMEN_CHECK( System.GetRole() == EActorRole::Standalone );
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Authority, Transport, Handler ) );
    LUMEN_CHECK_TRUE( System.IsInitialized() );
    LUMEN_CHECK( System.GetRole() == EActorRole::Authority );
    LUMEN_CHECK_FALSE( System.GetLastError().HasValue() );

    LUMEN_CHECK_HAS_VALUE( System.AssignPersistentId( 11ULL ) );
    LUMEN_CHECK_HAS_VALUE( System.AssignPersistentId( 12ULL ) );
    const TOptional<FPersistentActorId> First  = System.FindPersistentId( 11ULL );
    const TOptional<FPersistentActorId> Second = System.FindPersistentId( 12ULL );
    LUMEN_CHECK_HAS_VALUE( First );
    LUMEN_CHECK_HAS_VALUE( Second );
    LUMEN_CHECK_TRUE( First->IsValid() );
    LUMEN_CHECK_TRUE( *First != *Second );

    const TOptional<ActorID> Local = System.FindLocalId( *First );
    LUMEN_CHECK_HAS_VALUE( Local );
    LUMEN_CHECK_EQ( *Local, 11ULL );

    LUMEN_CHECK_HAS_VALUE( System.AssignPersistentId( 11ULL ) );
    const TOptional<FPersistentActorId> Again = System.FindPersistentId( 11ULL );
    LUMEN_CHECK_HAS_VALUE( Again );
    LUMEN_CHECK( *Again == *First );
    LUMEN_CHECK_EQ( *System.FindLocalId( *First ), 11ULL );
}

LUMEN_TEST( FReplicationAuthority, InvalidIdsAndReplicaOnlyOperationsAreRejected )
{
    FTestReplicationTransport Transport;
    FTestReplicationActorHandler Handler;
    FReplicationSystem System;
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Authority, Transport, Handler ) );

    LUMEN_CHECK_FALSE( System.FindLocalId( InvalidPersistentActorId ).HasValue() );
    LUMEN_CHECK_FALSE( System.FindPersistentId( InvalidActorID ).HasValue() );

    const TExpected<void, EReplicationError> InvalidAssign = System.AssignPersistentId( InvalidActorID );
    LUMEN_CHECK_HAS_ERROR( InvalidAssign );
    LUMEN_CHECK_EQ( InvalidAssign.Error(), EReplicationError::InvalidPersistentId );

    const TExpected<void, EReplicationError> RoleViolation = System.BindPersistentId( 11ULL, FPersistentActorId( 55ULL ) );
    LUMEN_CHECK_HAS_ERROR( RoleViolation );
    LUMEN_CHECK_EQ( RoleViolation.Error(), EReplicationError::AuthorityViolation );

    const TExpected<void, EReplicationError> InvalidVisibility = System.SetVisibility( InvalidPersistentActorId, false );
    LUMEN_CHECK_HAS_ERROR( InvalidVisibility );
    LUMEN_CHECK_EQ( InvalidVisibility.Error(), EReplicationError::InvalidPersistentId );

    const TExpected<void, EReplicationError> Unknown = System.SetVisibility( FPersistentActorId( 999ULL ), false );
    LUMEN_CHECK_HAS_ERROR( Unknown );
    LUMEN_CHECK_EQ( Unknown.Error(), EReplicationError::UnknownActor );

    LUMEN_CHECK_FALSE( System.FindLocalId( FPersistentActorId( 55ULL ) ).HasValue() );
}

LUMEN_TEST( FReplicationAuthority, VisibilityIsEditableForKnownIdsOnly )
{
    FTestReplicationTransport Transport;
    FTestReplicationActorHandler Handler;
    FReplicationSystem System;
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Authority, Transport, Handler ) );
    LUMEN_CHECK_HAS_VALUE( System.AssignPersistentId( 21ULL ) );
    const TOptional<FPersistentActorId> Id = System.FindPersistentId( 21ULL );
    LUMEN_CHECK_HAS_VALUE( Id );

    LUMEN_CHECK_TRUE( System.GetVisibility( *Id ) );
    LUMEN_CHECK_TRUE( System.GetVisibility( FPersistentActorId( 777ULL ) ) );
    LUMEN_CHECK_HAS_VALUE( System.SetVisibility( *Id, false ) );
    LUMEN_CHECK_FALSE( System.GetVisibility( *Id ) );
    LUMEN_CHECK_HAS_VALUE( System.SetVisibility( *Id, true ) );
    LUMEN_CHECK_TRUE( System.GetVisibility( *Id ) );
}

LUMEN_TEST( FReplicationAuthority, TickBuildsReliableFramesAndAcksDoNotTouchUnreliableCursor )
{
    FTestReplicationTransport Transport;
    FTestReplicationActorHandler Handler;
    FReplicationSystem System;
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Authority, Transport, Handler ) );
    LUMEN_CHECK_HAS_VALUE( System.AssignPersistentId( 31ULL ) );

    TVector<FReplicationActorSourceRecord> Actors;
    Actors.push_back( FReplicationActorSourceRecord{
        .LocalId = 31ULL, .ParentLocalId = InvalidActorID, .LocalTransform = Maths::FMat4x4f::Identity() } );
    const FReplicationSourceView Source = MakeSourceView( Actors, 1ULL );

    const TExpected<FReplicationTickResult, EReplicationError> Built = System.Tick( 1ULL, Source );
    LUMEN_CHECK_HAS_VALUE( Built );
    LUMEN_CHECK( Built->FramesSent >= 1ULL );
    FReplicationStats Stats = System.GetStats();
    LUMEN_CHECK( Stats.FramesBuilt >= 1ULL );
    LUMEN_CHECK( Stats.FullSnapshotsSent >= 1ULL );
    const USize StateIndex      = static_cast<USize>( EReplicationDirection::StateToReplica );
    const USize ReliableIndex   = static_cast<USize>( EReplicationChannel::Reliable );
    const USize UnreliableIndex = static_cast<USize>( EReplicationChannel::Unreliable );
    LUMEN_CHECK( Transport.GetPendingCount() >= 1ULL );
    LUMEN_CHECK_EQ( Stats.Channels[StateIndex][UnreliableIndex].SentCount, 0ULL );
    LUMEN_CHECK_EQ( Stats.Channels[StateIndex][UnreliableIndex].LastSeenSequence, 0ULL );
    DrainTransport( Transport );

    FReplicationRecord AckRecord;
    AckRecord.Kind = EActorReplicationKind::Control;
    AckRecord.SetControl( FReplicationControlPayload{ .Kind      = EReplicationControlKind::Ack,
                                                      .Direction = EReplicationDirection::StateToReplica,
                                                      .Channel   = EReplicationChannel::Reliable,
                                                      .Sequence  = 1ULL } );
    FReplicationFrameHeader AckHeader;
    AckHeader.AckedSequence               = 1ULL;
    AckHeader.Direction                   = EReplicationDirection::InputToAuthority;
    AckHeader.Channel                     = EReplicationChannel::Reliable;
    AckHeader.Flags                       = ReplicationFrameControlOnly;
    const FReplicationRecord AckRecords[] = { AckRecord };
    LUMEN_CHECK_HAS_VALUE( Transport.TrySend( MakeFrameBytes( AckHeader, AckRecords ) ) );

    LUMEN_CHECK_HAS_VALUE( System.Tick( 2ULL, Source ) );
    Stats = System.GetStats();
    LUMEN_CHECK( Stats.Channels[StateIndex][ReliableIndex].AckedSequence >= 1ULL );
    LUMEN_CHECK( Stats.LastAcknowledgedReliableSequence >= 1ULL );
    LUMEN_CHECK_EQ( Stats.Channels[StateIndex][UnreliableIndex].AckedSequence, 0ULL );
    LUMEN_CHECK_EQ( Stats.Channels[StateIndex][UnreliableIndex].LastSeenSequence, 0ULL );
}

LUMEN_TEST( FReplicationAuthority, ReceivesInputFramesIntoCopyReceivedInputs )
{
    FTestReplicationTransport Transport;
    FTestReplicationActorHandler Handler;
    FReplicationSystem System;
    LUMEN_CHECK_HAS_VALUE( System.Initialize( MakeValidConfig(), EActorRole::Authority, Transport, Handler ) );

    const Byte Payload[3] = { 9U, 8U, 7U };
    FReplicationRecord InputRecord;
    InputRecord.Actor = FPersistentActorId( 77ULL );
    InputRecord.Kind  = EActorReplicationKind::Input;
    InputRecord.SetInput( 55ULL, Payload, 3U );
    FReplicationFrameHeader Header;
    Header.Sequence                    = 1ULL;
    Header.Direction                   = EReplicationDirection::InputToAuthority;
    Header.Channel                     = EReplicationChannel::Reliable;
    Header.FrameId                     = 77ULL;
    const FReplicationRecord Records[] = { InputRecord };
    LUMEN_CHECK_HAS_VALUE( Transport.TrySend( MakeFrameBytes( Header, Records ) ) );

    const FReplicationSourceView Empty;
    LUMEN_CHECK_HAS_VALUE( System.Tick( 1ULL, Empty ) );
    LUMEN_CHECK( System.GetStats().FramesApplied >= 1ULL );

    TVector<FReplicationInputRecord> Copied;
    Copied.resize( 2ULL );
    const USize Count = System.CopyReceivedInputs( Copied );
    LUMEN_CHECK_EQ( Count, 1ULL );
    LUMEN_CHECK( Copied[0ULL].Actor == FPersistentActorId( 77ULL ) );
    LUMEN_CHECK_EQ( Copied[0ULL].InputSequence, 55ULL );
    LUMEN_CHECK_EQ( Copied[0ULL].PayloadSize, 3U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Copied[0ULL].Payload[0U] ), 9U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Copied[0ULL].Payload[2U] ), 7U );
    LUMEN_CHECK_EQ( System.CopyReceivedInputs( Copied ), 0ULL );
}

LUMEN_TEST( FReplicationAuthority, CapacityAndInitializationErrorsLeaveStateIntact )
{
    FTestReplicationTransport Transport;
    FTestReplicationActorHandler Handler;
    FReplicationSystem System;
    const FReplicationSourceView Empty;

    const TExpected<FReplicationTickResult, EReplicationError> PreTick = System.Tick( 1ULL, Empty );
    LUMEN_CHECK_HAS_ERROR( PreTick );
    LUMEN_CHECK_EQ( PreTick.Error(), EReplicationError::NotInitialized );
    const TExpected<void, EReplicationError> PreAssign = System.AssignPersistentId( 1ULL );
    LUMEN_CHECK_HAS_ERROR( PreAssign );
    LUMEN_CHECK_EQ( PreAssign.Error(), EReplicationError::NotInitialized );

    FReplicationConfig Invalid;
    const TExpected<void, EReplicationError> InvalidInit =
        System.Initialize( Invalid, EActorRole::Authority, Transport, Handler );
    LUMEN_CHECK_HAS_ERROR( InvalidInit );
    LUMEN_CHECK_EQ( InvalidInit.Error(), EReplicationError::InvalidCapacity );
    LUMEN_CHECK_FALSE( System.IsInitialized() );
    LUMEN_CHECK( System.GetRole() == EActorRole::Standalone );
    LUMEN_CHECK_FALSE( System.FindLocalId( FPersistentActorId( 1ULL ) ).HasValue() );
    LUMEN_CHECK_EQ( System.GetStats().FramesBuilt, 0ULL );

    const FReplicationConfig Config = MakeValidConfig();
    LUMEN_CHECK_HAS_VALUE( System.Initialize( Config, EActorRole::Authority, Transport, Handler ) );
    const TExpected<void, EReplicationError> Reinitialize = System.Initialize( Config, EActorRole::Replica, Transport, Handler );
    LUMEN_CHECK_HAS_ERROR( Reinitialize );
    LUMEN_CHECK_EQ( Reinitialize.Error(), EReplicationError::AlreadyInitialized );
    LUMEN_CHECK( System.GetRole() == EActorRole::Authority );

    for ( ActorID Local = 100ULL; Local < 108ULL; ++Local )
    {
        LUMEN_CHECK_HAS_VALUE( System.AssignPersistentId( Local ) );
    }
    const TExpected<void, EReplicationError> Overflow = System.AssignPersistentId( 108ULL );
    LUMEN_CHECK_HAS_ERROR( Overflow );
    LUMEN_CHECK_EQ( Overflow.Error(), EReplicationError::CapacityExceeded );
    LUMEN_CHECK_FALSE( System.FindPersistentId( 108ULL ).HasValue() );
    LUMEN_CHECK_TRUE( System.FindPersistentId( 100ULL ).HasValue() );
    LUMEN_CHECK_EQ( System.GetStats().FramesBuilt, 0ULL );
}

} // namespace Lumen
