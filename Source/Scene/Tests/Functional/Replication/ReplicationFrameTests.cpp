/**
 * @file ReplicationFrameTests.cpp
 * @brief Fixed-layout record, builder and little-endian wire codec contracts.
 */

#include <Replication/ReplicationFrame.hpp>

#include <Tests/UnitTest.hpp>

#include <Maths/Matrix.hpp>

#include <Templates/Vector.hpp>

namespace Lumen
{

namespace
{

    static_assert( sizeof( FReplicationRecord ) == 80ULL );
    static_assert( ReplicationFrameHeaderBytes == 56ULL );
    static_assert( ReplicationFrameRecordBytes == 80ULL );
    static_assert( ReplicationFrameChecksumBytes == 8ULL );
    static_assert( ReplicationFrameRecordBytes == MaxReplicationRecordBytes );

    /** @brief One frame of scratch: header, eight records and the trailing checksum. */
    [[nodiscard]] FReplicationFrameBuilder MakeReservedBuilder () noexcept
    {
        FReplicationFrameBuilder Builder;
        Builder.Reserve( ReplicationFrameHeaderBytes + ( 8ULL * ReplicationFrameRecordBytes ) + ReplicationFrameChecksumBytes,
                         8ULL );
        return Builder;
    }

    /** @brief Overwrites four bytes with a little-endian UInt32, used for malformed-frame fixtures. */
    void SetU32LE ( TSpan<Byte> InBytes, const USize InOffset, const UInt32 InValue ) noexcept
    {
        for ( UInt32 Index = 0U; Index < 4U; ++Index )
        {
            InBytes[InOffset + Index] = static_cast<Byte>( ( InValue >> ( Index * 8U ) ) & 0xFFU );
        }
    }

    /** @brief Builds the single-record frame used by corruption and lifetime checks. */
    void AppendSpawnRecord ( FReplicationFrameBuilder &InOutBuilder, const FPersistentActorId InId ) noexcept
    {
        FReplicationRecord Spawn;
        Spawn.Actor        = InId;
        Spawn.Kind         = EActorReplicationKind::Spawn;
        Spawn.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Alive );
        static_cast<void>( InOutBuilder.TryAppend( Spawn ) );
    }

    /** @brief Serializes the current builder into its own byte storage. */
    [[nodiscard]] TVector<Byte> WriteFrame ( const FReplicationFrameBuilder &InBuilder, const FReplicationFrameHeader &InHeader )
    {
        TVector<Byte> Bytes;
        Bytes.resize( ReplicationFrameHeaderBytes + ( 8ULL * ReplicationFrameRecordBytes ) + ReplicationFrameChecksumBytes );
        const TExpected<USize, EReplicationError> Written = InBuilder.TryWrite( Bytes, InHeader );
        LUMEN_CHECK_HAS_VALUE( Written );
        Bytes.resize( *Written );
        return Bytes;
    }

} // namespace

LUMEN_TEST( FReplicationFrame, RecordRoundTripsLocalTransform )
{
    const Maths::FMat4x4f Transform = Maths::FMat4x4f::Translate( Maths::FVec3f{ 3.0F, -4.0F, 5.0F } );

    FReplicationRecord Record;
    LUMEN_CHECK_EQ( Record.PayloadSize, 0U );
    Record.SetLocalTransform( Transform );
    LUMEN_CHECK_EQ( Record.PayloadSize, static_cast<UInt16>( sizeof( Maths::FMat4x4f ) ) );
    LUMEN_CHECK( Record.GetLocalTransform() == Transform );

    Record.SetLocalTransform( Maths::FMat4x4f::Identity() );
    LUMEN_CHECK( Record.GetLocalTransform() == Maths::FMat4x4f::Identity() );
}

LUMEN_TEST( FReplicationFrame, RecordRoundTripsParentAndVisibility )
{
    FReplicationRecord Record;
    LUMEN_CHECK_FALSE( Record.GetVisibility() );
    LUMEN_CHECK( Record.GetParent() == InvalidPersistentActorId );

    const FPersistentActorId Parent( 77ULL );
    Record.SetParent( Parent );
    LUMEN_CHECK_EQ( Record.PayloadSize, 8U );
    LUMEN_CHECK( Record.GetParent() == Parent );

    Record.SetParent( InvalidPersistentActorId );
    LUMEN_CHECK( Record.GetParent() == InvalidPersistentActorId );

    Record.SetVisibility( true );
    LUMEN_CHECK_EQ( Record.PayloadSize, 1U );
    LUMEN_CHECK_TRUE( Record.GetVisibility() );

    Record.SetVisibility( false );
    LUMEN_CHECK_EQ( Record.PayloadSize, 1U );
    LUMEN_CHECK_FALSE( Record.GetVisibility() );
}

LUMEN_TEST( FReplicationFrame, RecordRoundTripsEveryControlKind )
{
    const EReplicationControlKind Kinds[] = { EReplicationControlKind::Ack, EReplicationControlKind::Nack,
                                              EReplicationControlKind::BaselineRequest, EReplicationControlKind::BaselineReady };

    for ( const EReplicationControlKind Kind : Kinds )
    {
        const FReplicationControlPayload Control{ .Kind      = Kind,
                                                  .Direction = EReplicationDirection::InputToAuthority,
                                                  .Channel   = EReplicationChannel::Unreliable,
                                                  .Sequence  = 123456789ULL };
        FReplicationRecord Record;
        Record.Kind = EActorReplicationKind::Control;
        Record.SetControl( Control );
        LUMEN_CHECK_EQ( Record.PayloadSize, static_cast<UInt16>( sizeof( FReplicationControlPayload ) ) );

        const FReplicationControlPayload Decoded = Record.GetControl();
        LUMEN_CHECK( Decoded.Kind == Control.Kind );
        LUMEN_CHECK( Decoded.Direction == Control.Direction );
        LUMEN_CHECK( Decoded.Channel == Control.Channel );
        LUMEN_CHECK_EQ( Decoded.Sequence, Control.Sequence );
    }

    FReplicationRecord DefaultRecord;
    const FReplicationControlPayload DefaultControl = DefaultRecord.GetControl();
    LUMEN_CHECK( DefaultControl.Kind == EReplicationControlKind::Ack );
    LUMEN_CHECK_EQ( DefaultControl.Sequence, 0ULL );
}

LUMEN_TEST( FReplicationFrame, RecordInputPayloadBoundariesAndTruncation )
{
    const Byte Data[MaxReplicationInputPayloadBytes] = { 10U, 20U, 30U, 40U };

    FReplicationRecord Record;
    Record.Kind = EActorReplicationKind::Input;
    Record.SetInput( 91ULL, Data, 4U );
    LUMEN_CHECK_EQ( Record.GetInputSequence(), 91ULL );
    LUMEN_CHECK_EQ( Record.PayloadSize, 14U );
    LUMEN_CHECK_EQ( Record.GetInputPayload().size(), 4ULL );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Record.GetInputPayload()[0U] ), 10U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Record.GetInputPayload()[3U] ), 40U );

    FReplicationRecord Empty;
    Empty.SetInput( 0ULL, nullptr, 0U );
    LUMEN_CHECK_EQ( Empty.GetInputSequence(), 0ULL );
    LUMEN_CHECK_TRUE( Empty.GetInputPayload().empty() );

    FReplicationRecord Full;
    Full.SetInput( 0xFFFFFFFFFFFFFFFFULL, Data, static_cast<UInt16>( MaxReplicationInputPayloadBytes ) );
    LUMEN_CHECK_EQ( Full.GetInputSequence(), 0xFFFFFFFFFFFFFFFFULL );
    LUMEN_CHECK_EQ( Full.GetInputPayload().size(), MaxReplicationInputPayloadBytes );

    FReplicationRecord Truncated;
    Truncated.PayloadSize = 8U;
    LUMEN_CHECK_TRUE( Truncated.GetInputPayload().empty() );

    FReplicationRecord InvalidDeclaredSize;
    InvalidDeclaredSize.SetInput( 5ULL, Data, 4U );
    InvalidDeclaredSize.Payload[8U] = 0xFFU;
    InvalidDeclaredSize.Payload[9U] = 0xFFU;
    LUMEN_CHECK_TRUE( InvalidDeclaredSize.GetInputPayload().empty() );
    LUMEN_CHECK_EQ( InvalidDeclaredSize.GetInputSequence(), 5ULL );
}

LUMEN_TEST( FReplicationFrame, BuilderReserveResetAndCapacityOverflow )
{
    FReplicationFrameBuilder Builder = MakeReservedBuilder();
    LUMEN_CHECK_EQ( Builder.GetByteCapacity(),
                    ReplicationFrameHeaderBytes + ( 8ULL * ReplicationFrameRecordBytes ) + ReplicationFrameChecksumBytes );
    LUMEN_CHECK_EQ( Builder.GetRecordCapacity(), 8ULL );
    LUMEN_CHECK_TRUE( Builder.GetRecords().empty() );

    for ( USize Index = 0ULL; Index < 8ULL; ++Index )
    {
        FReplicationRecord Record;
        Record.Actor = FPersistentActorId( Index + 1ULL );
        Record.Kind  = EActorReplicationKind::Spawn;
        LUMEN_CHECK_HAS_VALUE( Builder.TryAppend( Record ) );
    }
    LUMEN_CHECK_EQ( Builder.GetRecords().size(), 8ULL );

    FReplicationRecord Overflow;
    Overflow.Actor                                    = FPersistentActorId( 100ULL );
    const TExpected<void, EReplicationError> Rejected = Builder.TryAppend( Overflow );
    LUMEN_CHECK_HAS_ERROR( Rejected );
    LUMEN_CHECK_EQ( Rejected.Error(), EReplicationError::CapacityExceeded );
    LUMEN_CHECK_EQ( Builder.GetRecords().size(), 8ULL );
    LUMEN_CHECK( Builder.GetRecords()[7ULL].Actor == FPersistentActorId( 8ULL ) );

    Builder.Reset();
    LUMEN_CHECK_TRUE( Builder.GetRecords().empty() );
    LUMEN_CHECK_EQ( Builder.GetRecordCapacity(), 8ULL );
    LUMEN_CHECK_HAS_VALUE( Builder.TryAppend( Overflow ) );
    LUMEN_CHECK_EQ( Builder.GetRecords().size(), 1ULL );

    Builder.Reserve( 1ULL, 1ULL );
    LUMEN_CHECK_EQ( Builder.GetRecordCapacity(), 8ULL );
}

LUMEN_TEST( FReplicationFrame, WriteParseRoundTripPreservesHeaderAndRecords )
{
    FReplicationFrameBuilder Builder = MakeReservedBuilder();
    AppendSpawnRecord( Builder, FPersistentActorId( 11ULL ) );

    FReplicationRecord Transform;
    Transform.Actor        = FPersistentActorId( 11ULL );
    Transform.Kind         = EActorReplicationKind::Transform;
    Transform.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::LocalTransform );
    Transform.SetLocalTransform( Maths::FMat4x4f::Translate( Maths::FVec3f{ 1.0F, 2.0F, 3.0F } ) );
    LUMEN_CHECK_HAS_VALUE( Builder.TryAppend( Transform ) );

    FReplicationRecord Visibility;
    Visibility.Actor        = FPersistentActorId( 11ULL );
    Visibility.Kind         = EActorReplicationKind::Visibility;
    Visibility.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Visibility );
    Visibility.SetVisibility( false );
    LUMEN_CHECK_HAS_VALUE( Builder.TryAppend( Visibility ) );

    FReplicationFrameHeader Header;
    Header.Sequence         = 7ULL;
    Header.AckedSequence    = 6ULL;
    Header.BaselineSequence = 5ULL;
    Header.FrameId          = 44ULL;
    Header.Direction        = EReplicationDirection::StateToReplica;
    Header.Channel          = EReplicationChannel::Reliable;
    Header.Flags            = ReplicationFrameFullSnapshot;

    const TVector<Byte> Bytes = WriteFrame( Builder, Header );
    LUMEN_CHECK_EQ( Bytes.size(),
                    ReplicationFrameHeaderBytes + ( 3ULL * ReplicationFrameRecordBytes ) + ReplicationFrameChecksumBytes );

    FReplicationFrameBuilder Parser                              = MakeReservedBuilder();
    const TExpected<FReplicationFrame, EReplicationError> Parsed = Parser.TryParse( Bytes );
    LUMEN_CHECK_HAS_VALUE( Parsed );
    LUMEN_CHECK_EQ( Parsed->Header.Sequence, 7ULL );
    LUMEN_CHECK_EQ( Parsed->Header.AckedSequence, 6ULL );
    LUMEN_CHECK_EQ( Parsed->Header.BaselineSequence, 5ULL );
    LUMEN_CHECK_EQ( Parsed->Header.FrameId, 44ULL );
    LUMEN_CHECK( Parsed->Header.Direction == EReplicationDirection::StateToReplica );
    LUMEN_CHECK( Parsed->Header.Channel == EReplicationChannel::Reliable );
    LUMEN_CHECK_EQ( Parsed->Header.RecordCount, 3U );
    LUMEN_CHECK_EQ( Parsed->Header.Flags, ReplicationFrameFullSnapshot );
    LUMEN_CHECK_EQ( Parsed->Header.ByteLength, static_cast<UInt32>( Bytes.size() ) );
    LUMEN_CHECK_EQ( Parsed->Records.size(), 3ULL );
    LUMEN_CHECK( Parsed->Records[0ULL].Actor == FPersistentActorId( 11ULL ) );
    LUMEN_CHECK( Parsed->Records[0ULL].Kind == EActorReplicationKind::Spawn );
    LUMEN_CHECK( Parsed->Records[1ULL].Kind == EActorReplicationKind::Transform );
    LUMEN_CHECK( Parsed->Records[1ULL].GetLocalTransform() == Transform.GetLocalTransform() );
    LUMEN_CHECK( Parsed->Records[2ULL].Kind == EActorReplicationKind::Visibility );
    LUMEN_CHECK_FALSE( Parsed->Records[2ULL].GetVisibility() );
}

LUMEN_TEST( FReplicationFrame, ParseRejectsTruncatedAndCorruptedFrames )
{
    FReplicationFrameBuilder Builder = MakeReservedBuilder();
    AppendSpawnRecord( Builder, FPersistentActorId( 11ULL ) );
    const TVector<Byte> Valid = WriteFrame( Builder, FReplicationFrameHeader{} );

    FReplicationFrameBuilder Parser                             = MakeReservedBuilder();
    const TExpected<FReplicationFrame, EReplicationError> Short = Parser.TryParse( { Valid.data(), Valid.size() - 1ULL } );
    LUMEN_CHECK_HAS_ERROR( Short );
    LUMEN_CHECK_EQ( Short.Error(), EReplicationError::MalformedFrame );

    const TExpected<FReplicationFrame, EReplicationError> HeaderOnly =
        Parser.TryParse( { Valid.data(), ReplicationFrameHeaderBytes + ReplicationFrameChecksumBytes - 1ULL } );
    LUMEN_CHECK_HAS_ERROR( HeaderOnly );
    LUMEN_CHECK_EQ( HeaderOnly.Error(), EReplicationError::MalformedFrame );

    TVector<Byte> WrongLength = Valid;
    SetU32LE( WrongLength, 40U, static_cast<UInt32>( Valid.size() + 1ULL ) );
    const TExpected<FReplicationFrame, EReplicationError> BadLength = Parser.TryParse( WrongLength );
    LUMEN_CHECK_HAS_ERROR( BadLength );

    TVector<Byte> TooManyRecords                                   = Valid;
    TooManyRecords.at( 34U )                                       = 9U;
    const TExpected<FReplicationFrame, EReplicationError> BadCount = Parser.TryParse( TooManyRecords );
    LUMEN_CHECK_HAS_ERROR( BadCount );

    TVector<Byte> BadDirection                                             = Valid;
    BadDirection.at( 32U )                                                 = 2U;
    const TExpected<FReplicationFrame, EReplicationError> InvalidDirection = Parser.TryParse( BadDirection );
    LUMEN_CHECK_HAS_ERROR( InvalidDirection );

    TVector<Byte> BadChannel                                             = Valid;
    BadChannel.at( 33U )                                                 = 2U;
    const TExpected<FReplicationFrame, EReplicationError> InvalidChannel = Parser.TryParse( BadChannel );
    LUMEN_CHECK_HAS_ERROR( InvalidChannel );

    TVector<Byte> BadChecksum                                       = Valid;
    const Byte Flipped                                              = static_cast<Byte>( BadChecksum.at( 48U ) ^ Byte{ 0xFFU } );
    BadChecksum.at( 48U )                                           = Flipped;
    const TExpected<FReplicationFrame, EReplicationError> Corrupted = Parser.TryParse( BadChecksum );
    LUMEN_CHECK_HAS_ERROR( Corrupted );

    const TExpected<FReplicationFrame, EReplicationError> Restored = Parser.TryParse( Valid );
    LUMEN_CHECK_HAS_VALUE( Restored );
}

LUMEN_TEST( FReplicationFrame, WriteRejectsOutputSmallerThanDeclaredFrame )
{
    FReplicationFrameBuilder Builder = MakeReservedBuilder();
    AppendSpawnRecord( Builder, FPersistentActorId( 11ULL ) );

    FReplicationFrameHeader Header;
    Header.Sequence = 3ULL;

    TVector<Byte> ShortBytes;
    ShortBytes.resize( ReplicationFrameHeaderBytes + ReplicationFrameRecordBytes + ReplicationFrameChecksumBytes - 1ULL );
    const TExpected<USize, EReplicationError> Rejected = Builder.TryWrite( ShortBytes, Header );
    LUMEN_CHECK_HAS_ERROR( Rejected );
    LUMEN_CHECK_EQ( Rejected.Error(), EReplicationError::CapacityExceeded );

    TVector<Byte> Bytes;
    Bytes.resize( ReplicationFrameHeaderBytes + ReplicationFrameRecordBytes + ReplicationFrameChecksumBytes );
    const TExpected<USize, EReplicationError> Written = Builder.TryWrite( Bytes, Header );
    LUMEN_CHECK_HAS_VALUE( Written );
    LUMEN_CHECK_EQ( *Written, Bytes.size() );
}

LUMEN_TEST( FReplicationFrame, ParsedRecordsStayValidUntilBuilderReset )
{
    FReplicationFrameBuilder Builder = MakeReservedBuilder();
    AppendSpawnRecord( Builder, FPersistentActorId( 42ULL ) );
    const TVector<Byte> Bytes = WriteFrame( Builder, FReplicationFrameHeader{} );

    FReplicationFrameBuilder Parser                              = MakeReservedBuilder();
    const TExpected<FReplicationFrame, EReplicationError> Parsed = Parser.TryParse( Bytes );
    LUMEN_CHECK_HAS_VALUE( Parsed );
    const TSpan<const FReplicationRecord> Held = Parsed->Records;
    LUMEN_CHECK( Held[0ULL].Actor == FPersistentActorId( 42ULL ) );

    for ( USize Index = 0ULL; Index < 3ULL; ++Index )
    {
        FReplicationRecord Extra;
        Extra.Actor = FPersistentActorId( 100ULL + Index );
        LUMEN_CHECK_HAS_VALUE( Parser.TryAppend( Extra ) );
        LUMEN_CHECK( Held[0ULL].Actor == FPersistentActorId( 42ULL ) );
    }

    Parser.Reset();
    LUMEN_CHECK_TRUE( Parser.GetRecords().empty() );
}

LUMEN_TEST( FReplicationFrame, CanonicalLayoutIsLittleEndian )
{
    FReplicationFrameBuilder Builder = MakeReservedBuilder();
    FReplicationRecord Record;
    Record.Actor        = FPersistentActorId( 0xAABBCCDDEEFF0011ULL );
    Record.Kind         = EActorReplicationKind::Transform;
    Record.PropertyRank = 2U;
    Record.SetVisibility( true );
    LUMEN_CHECK_HAS_VALUE( Builder.TryAppend( Record ) );

    FReplicationFrameHeader Header;
    Header.Sequence      = 0x0102030405060708ULL;
    Header.AckedSequence = 0x1122334455667788ULL;
    Header.FrameId       = 0x8877665544332211ULL;
    Header.Direction     = EReplicationDirection::InputToAuthority;
    Header.Channel       = EReplicationChannel::Unreliable;
    Header.Flags         = static_cast<UInt16>( ReplicationFrameFullSnapshot | ReplicationFrameControlOnly );

    const TVector<Byte> Bytes = WriteFrame( Builder, Header );
    LUMEN_CHECK_EQ( Bytes.size(), ReplicationFrameHeaderBytes + ReplicationFrameRecordBytes + ReplicationFrameChecksumBytes );

    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[0U] ), 8U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[7U] ), 1U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[8U] ), 0x88U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[15U] ), 0x11U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[24U] ), 0x11U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[31U] ), 0x88U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[32U] ), 1U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[33U] ), 1U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[34U] ), 1U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[35U] ), 0U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[36U] ), 3U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[40U] ), static_cast<UInt32>( Bytes.size() ) );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[43U] ), 0U );

    const USize RecordOffset = ReplicationFrameHeaderBytes;
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[RecordOffset] ), 0x11U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[RecordOffset + 7ULL] ), 0xAAU );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[RecordOffset + 8ULL] ), static_cast<UInt32>( EActorReplicationKind::Transform ) );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[RecordOffset + 9ULL] ), 2U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[RecordOffset + 10ULL] ), 1U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Bytes[RecordOffset + 11ULL] ), 0U );

    FReplicationFrameBuilder Parser                              = MakeReservedBuilder();
    const TExpected<FReplicationFrame, EReplicationError> Parsed = Parser.TryParse( Bytes );
    LUMEN_CHECK_HAS_VALUE( Parsed );
    LUMEN_CHECK_EQ( Parsed->Header.Sequence, Header.Sequence );
    LUMEN_CHECK_EQ( Parsed->Header.FrameId, Header.FrameId );
    LUMEN_CHECK_EQ( Parsed->Header.Flags, Header.Flags );
    LUMEN_CHECK( Parsed->Records[0ULL].Actor == FPersistentActorId( 0xAABBCCDDEEFF0011ULL ) );
}

} // namespace Lumen
