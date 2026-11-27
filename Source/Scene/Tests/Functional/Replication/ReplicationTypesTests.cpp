/**
 * @file ReplicationTypesTests.cpp
 * @brief Value, rank and capacity contracts of the replication wire vocabulary.
 */

#include <Replication/ReplicationTypes.hpp>

#include <Tests/UnitTest.hpp>

#include <Concepts/TriviallyCopyable.hpp>

namespace Lumen
{

namespace
{

    /** @brief Smallest valid bounded configuration used throughout these contract tests. */
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

    /** @brief Asserts that a configuration is rejected with InvalidCapacity. */
    void ExpectInvalidCapacity ( const FReplicationConfig &InConfig )
    {
        const TExpected<void, EReplicationError> Result = InConfig.Validate();
        LUMEN_CHECK_HAS_ERROR( Result );
        LUMEN_CHECK_EQ( Result.Error(), EReplicationError::InvalidCapacity );
    }

} // namespace

LUMEN_TEST( FReplicationTypes, PersistentIdZeroInvalidAndOrdering )
{
    static_assert( Concepts::CTriviallyCopyable<FPersistentActorId> );
    static_assert( sizeof( FPersistentActorId ) == sizeof( UInt64 ) );

    const FPersistentActorId DefaultId;
    LUMEN_CHECK_FALSE( DefaultId.IsValid() );
    LUMEN_CHECK_EQ( DefaultId.GetValue(), 0ULL );
    LUMEN_CHECK_TRUE( DefaultId == InvalidPersistentActorId );

    const FPersistentActorId First( 1ULL );
    const FPersistentActorId Second( 2ULL );
    LUMEN_CHECK_TRUE( First.IsValid() );
    LUMEN_CHECK_EQ( First.GetValue(), 1ULL );
    LUMEN_CHECK_TRUE( First < Second );
    LUMEN_CHECK_TRUE( Second > First );
    LUMEN_CHECK_TRUE( First == FPersistentActorId( 1ULL ) );
    LUMEN_CHECK_TRUE( First != Second );
    LUMEN_CHECK_TRUE( DefaultId < First );
}

LUMEN_TEST( FReplicationTypes, DirtyMaskMarkUnmarkClearAndRankBits )
{
    static_assert( Concepts::CTriviallyCopyable<FDirtyPropertyMask> );

    FDirtyPropertyMask Mask;
    LUMEN_CHECK_TRUE( Mask.IsEmpty() );
    LUMEN_CHECK_EQ( Mask.GetBits(), 0U );

    Mask.Mark( EActorReplicationProperty::Alive );
    Mask.Mark( EActorReplicationProperty::LocalTransform );
    Mask.Mark( EActorReplicationProperty::Parent );
    Mask.Mark( EActorReplicationProperty::Visibility );
    LUMEN_CHECK_FALSE( Mask.IsEmpty() );
    LUMEN_CHECK_EQ( Mask.GetBits(), 0x0FU );
    LUMEN_CHECK_TRUE( Mask.Has( EActorReplicationProperty::Alive ) );
    LUMEN_CHECK_TRUE( Mask.Has( EActorReplicationProperty::LocalTransform ) );
    LUMEN_CHECK_TRUE( Mask.Has( EActorReplicationProperty::Parent ) );
    LUMEN_CHECK_TRUE( Mask.Has( EActorReplicationProperty::Visibility ) );

    Mask.Mark( EActorReplicationProperty::Alive );
    LUMEN_CHECK_EQ( Mask.GetBits(), 0x0FU );

    Mask.Unmark( EActorReplicationProperty::Parent );
    LUMEN_CHECK_FALSE( Mask.Has( EActorReplicationProperty::Parent ) );
    LUMEN_CHECK_EQ( Mask.GetBits(), 0x0BU );

    Mask.Clear();
    LUMEN_CHECK_TRUE( Mask.IsEmpty() );
    LUMEN_CHECK_EQ( Mask.GetBits(), 0U );
    LUMEN_CHECK_FALSE( Mask.Has( EActorReplicationProperty::Alive ) );
}

LUMEN_TEST( FReplicationTypes, DirtyMaskRanksAreStableAcrossFullWidth )
{
    static_assert( static_cast<UInt8>( EActorReplicationProperty::Alive ) == 0U );
    static_assert( static_cast<UInt8>( EActorReplicationProperty::LocalTransform ) == 1U );
    static_assert( static_cast<UInt8>( EActorReplicationProperty::Parent ) == 2U );
    static_assert( static_cast<UInt8>( EActorReplicationProperty::Visibility ) == 3U );
    static_assert( MaxReplicationPropertyRanks == 32U );

    for ( UInt32 Rank = 0U; Rank < static_cast<UInt32>( MaxReplicationPropertyRanks ); ++Rank )
    {
        FDirtyPropertyMask Single;
        const EActorReplicationProperty Property = static_cast<EActorReplicationProperty>( static_cast<UInt8>( Rank ) );
        Single.Mark( Property );
        LUMEN_CHECK_EQ( Single.GetBits(), 1U << Rank );
        LUMEN_CHECK_TRUE( Single.Has( Property ) );

        Single.Unmark( Property );
        LUMEN_CHECK_TRUE( Single.IsEmpty() );
    }

    FDirtyPropertyMask AllRanks;
    AllRanks.Mark( static_cast<EActorReplicationProperty>( 31U ) );
    LUMEN_CHECK_EQ( AllRanks.GetBits(), 0x80000000U );
}

LUMEN_TEST( FReplicationConfig, ValidateAcceptsBoundedCapacities )
{
    const FReplicationConfig Config                 = MakeValidConfig();
    const TExpected<void, EReplicationError> Result = Config.Validate();
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Config.MaxRecordsPerFrame, Config.MaxActors * MaxReplicationRecordsPerActor );
    LUMEN_CHECK_EQ( Config.MaxPayloadBytes, Config.MaxActors * MaxReplicationRecordsPerActor * MaxReplicationRecordBytes );
}

LUMEN_TEST( FReplicationConfig, ValidateRejectsZeroCapacitiesAndTooSmallPayload )
{
    ExpectInvalidCapacity( FReplicationConfig{} );
    ExpectInvalidCapacity( FReplicationConfig{ .MaxActors = 8ULL } );

    FReplicationConfig Config = MakeValidConfig();
    Config.MaxActors          = 0ULL;
    ExpectInvalidCapacity( Config );

    Config                    = MakeValidConfig();
    Config.MaxRecordsPerFrame = 0ULL;
    ExpectInvalidCapacity( Config );

    Config                 = MakeValidConfig();
    Config.MaxPayloadBytes = 0ULL;
    ExpectInvalidCapacity( Config );

    Config              = MakeValidConfig();
    Config.ResendWindow = 0ULL;
    ExpectInvalidCapacity( Config );

    Config                   = MakeValidConfig();
    Config.MaxBufferedFrames = 0ULL;
    ExpectInvalidCapacity( Config );

    Config                          = MakeValidConfig();
    Config.MaxPendingControlRecords = 0ULL;
    ExpectInvalidCapacity( Config );

    Config                   = MakeValidConfig();
    Config.MaxPendingParents = 0ULL;
    ExpectInvalidCapacity( Config );

    Config                  = MakeValidConfig();
    Config.MaxPendingInputs = 0ULL;
    ExpectInvalidCapacity( Config );

    Config                   = MakeValidConfig();
    Config.MaxResendAttempts = 0U;
    ExpectInvalidCapacity( Config );

    Config                           = MakeValidConfig();
    Config.FullSnapshotTickThreshold = 0U;
    ExpectInvalidCapacity( Config );

    Config                           = MakeValidConfig();
    Config.FullSnapshotCooldownTicks = 0U;
    ExpectInvalidCapacity( Config );

    Config                       = MakeValidConfig();
    Config.MaxPendingParentTicks = 0U;
    ExpectInvalidCapacity( Config );

    Config                 = MakeValidConfig();
    Config.MaxPayloadBytes = ( Config.MaxActors * MaxReplicationRecordsPerActor * MaxReplicationRecordBytes ) - 1ULL;
    ExpectInvalidCapacity( Config );

    Config                    = MakeValidConfig();
    Config.MaxRecordsPerFrame = ( Config.MaxActors * MaxReplicationRecordsPerActor ) - 1ULL;
    ExpectInvalidCapacity( Config );

    Config                    = MakeValidConfig();
    Config.MaxRecordsPerFrame = Config.MaxActors + 1ULL;
    ExpectInvalidCapacity( Config );
}

LUMEN_TEST( FReplicationTypes, PropertyRanksReserveStableRanges )
{
    static_assert( static_cast<UInt8>( EActorReplicationProperty::Count ) <= 16U );
    static_assert( static_cast<UInt8>( EActorReplicationProperty::M9ReservedFirst ) == 4U );
    static_assert( static_cast<UInt8>( EActorReplicationProperty::M9ReservedLast ) == 15U );
    static_assert( static_cast<UInt8>( EActorReplicationProperty::M10ReservedFirst ) == 16U );
    static_assert( static_cast<UInt8>( EActorReplicationProperty::M10ReservedLast ) == 31U );

    LUMEN_CHECK_EQ( static_cast<UInt8>( EActorReplicationProperty::Alive ), 0U );
    LUMEN_CHECK_EQ( static_cast<UInt8>( EActorReplicationProperty::Count ), 4U );
}

LUMEN_TEST( FReplicationTypes, EnumsAndChannelIndexHaveExplicitValues )
{
    static_assert( static_cast<UInt8>( EActorRole::Standalone ) == 0U );
    static_assert( static_cast<UInt8>( EActorRole::Authority ) == 1U );
    static_assert( static_cast<UInt8>( EActorRole::Replica ) == 2U );
    static_assert( static_cast<UInt8>( EReplicationDirection::StateToReplica ) == 0U );
    static_assert( static_cast<UInt8>( EReplicationDirection::InputToAuthority ) == 1U );
    static_assert( static_cast<UInt8>( EReplicationChannel::Reliable ) == 0U );
    static_assert( static_cast<UInt8>( EReplicationChannel::Unreliable ) == 1U );
    static_assert( static_cast<UInt8>( EActorReplicationKind::Spawn ) == 0U );
    static_assert( static_cast<UInt8>( EActorReplicationKind::Destroy ) == 1U );
    static_assert( static_cast<UInt8>( EActorReplicationKind::Transform ) == 2U );
    static_assert( static_cast<UInt8>( EActorReplicationKind::Parent ) == 3U );
    static_assert( static_cast<UInt8>( EActorReplicationKind::Visibility ) == 4U );
    static_assert( static_cast<UInt8>( EActorReplicationKind::Input ) == 5U );
    static_assert( static_cast<UInt8>( EActorReplicationKind::Control ) == 6U );
    static_assert( static_cast<UInt8>( EActorReplicationKind::Count ) == 7U );
    static_assert( static_cast<UInt8>( EReplicationControlKind::Ack ) == 0U );
    static_assert( static_cast<UInt8>( EReplicationControlKind::Nack ) == 1U );
    static_assert( static_cast<UInt8>( EReplicationControlKind::BaselineRequest ) == 2U );
    static_assert( static_cast<UInt8>( EReplicationControlKind::BaselineReady ) == 3U );
    static_assert( static_cast<UInt8>( EReplicationError::None ) == 0U );
    static_assert( static_cast<UInt8>( EReplicationError::AlreadyInitialized ) == 2U );
    static_assert( static_cast<UInt8>( EReplicationError::JournalFull ) == 14U );

    LUMEN_CHECK_EQ( ReplicationChannelIndex( EReplicationDirection::StateToReplica, EReplicationChannel::Reliable ), 0ULL );
    LUMEN_CHECK_EQ( ReplicationChannelIndex( EReplicationDirection::StateToReplica, EReplicationChannel::Unreliable ), 1ULL );
    LUMEN_CHECK_EQ( ReplicationChannelIndex( EReplicationDirection::InputToAuthority, EReplicationChannel::Reliable ), 2ULL );
    LUMEN_CHECK_EQ( ReplicationChannelIndex( EReplicationDirection::InputToAuthority, EReplicationChannel::Unreliable ), 3ULL );
}

LUMEN_TEST( FReplicationTypes, PlainWirePodsHaveDocumentedDefaults )
{
    static_assert( Concepts::CTriviallyCopyable<FReplicationControlPayload> );
    static_assert( Concepts::CTriviallyCopyable<FDirtyPropertyMask> );
    static_assert( Concepts::CTriviallyCopyable<FReplicationActorSourceRecord> );
    static_assert( Concepts::CTriviallyCopyable<FReplicationInputRecord> );
    static_assert( sizeof( FReplicationInputRecord ) >= MaxReplicationInputPayloadBytes );

    const FReplicationControlPayload Control;
    LUMEN_CHECK( Control.Kind == EReplicationControlKind::Ack );
    LUMEN_CHECK( Control.Direction == EReplicationDirection::StateToReplica );
    LUMEN_CHECK( Control.Channel == EReplicationChannel::Reliable );
    LUMEN_CHECK_EQ( Control.Sequence, 0ULL );

    FReplicationStats Stats;
    LUMEN_CHECK_EQ( Stats.FramesBuilt, 0ULL );
    LUMEN_CHECK_EQ( Stats.Channels[0][0].NextOutgoingSequence, 1ULL );
    LUMEN_CHECK_EQ( Stats.Channels[1][1].NextOutgoingSequence, 1ULL );
    LUMEN_CHECK_EQ( Stats.Channels[1][1].AckedSequence, 0ULL );

    const FReplicationActorSourceRecord Source;
    LUMEN_CHECK( Source.LocalId == InvalidActorID );
    LUMEN_CHECK( Source.ParentLocalId == InvalidActorID );
    LUMEN_CHECK( Source.LocalTransform == Maths::FMat4x4f::Identity() );

    const FReplicationSourceView View;
    LUMEN_CHECK_EQ( View.Actors.size(), 0ULL );
    LUMEN_CHECK_EQ( View.FrameIndex, 0ULL );

    const FReplicationTickResult Result;
    LUMEN_CHECK_EQ( Result.FramesSent, 0ULL );
    LUMEN_CHECK_EQ( Result.FramesReceived, 0ULL );
    LUMEN_CHECK_EQ( Result.RecordsApplied, 0ULL );
    LUMEN_CHECK_EQ( Result.ControlRecordsSent, 0ULL );
    LUMEN_CHECK_FALSE( Result.bOverflow );

    /** INFO: Compile-existence smoke check; the interface is abstract and has no standalone state. */
    IReplicationActorHandler *const Handler = nullptr;
    LUMEN_CHECK( Handler == nullptr );
}

} // namespace Lumen
