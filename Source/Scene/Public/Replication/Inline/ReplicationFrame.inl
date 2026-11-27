/**
 * @file ReplicationFrame.inl
 * @brief Inline record helpers and little-endian frame codec.
 */

#pragma once

#include <Replication/ReplicationFrame.hpp>

#include <Templates/Unexpected.hpp>

#include <cstring>

namespace Lumen
{
namespace
{

    inline constexpr UInt64 FNVOffsetBasis = 0xCBF29CE484222325ULL;
    inline constexpr UInt64 FNVPrime       = 0x100000001B3ULL;

    inline void WriteU64 ( Byte *InDestination, const UInt64 InValue ) noexcept
    {
        for ( UInt32 Index = 0U; Index < 8U; ++Index )
        {
            InDestination[Index] = static_cast<Byte>( ( InValue >> ( Index * 8U ) ) & 0xFFULL );
        }
    }

    inline void WriteU32 ( Byte *InDestination, const UInt32 InValue ) noexcept
    {
        for ( UInt32 Index = 0U; Index < 4U; ++Index )
        {
            InDestination[Index] = static_cast<Byte>( ( InValue >> ( Index * 8U ) ) & 0xFFU );
        }
    }

    inline void WriteU16 ( Byte *InDestination, const UInt16 InValue ) noexcept
    {
        InDestination[0U] = static_cast<Byte>( InValue & 0xFFU );
        InDestination[1U] = static_cast<Byte>( ( InValue >> 8U ) & 0xFFU );
    }

    inline UInt64 ReadU64 ( const Byte *InSource ) noexcept
    {
        UInt64 Value = 0ULL;
        for ( UInt32 Index = 0U; Index < 8U; ++Index )
        {
            Value |= static_cast<UInt64>( InSource[Index] ) << ( Index * 8U );
        }
        return Value;
    }

    inline UInt32 ReadU32 ( const Byte *InSource ) noexcept
    {
        UInt32 Value = 0U;
        for ( UInt32 Index = 0U; Index < 4U; ++Index )
        {
            Value |= static_cast<UInt32>( InSource[Index] ) << ( Index * 8U );
        }
        return Value;
    }

    inline UInt16 ReadU16 ( const Byte *InSource ) noexcept
    {
        return static_cast<UInt16>( static_cast<UInt16>( InSource[0U] ) |
                                    static_cast<UInt16>( static_cast<UInt16>( InSource[1U] ) << 8U ) );
    }

    inline UInt64 HashBytes ( UInt64 InSeed, const Byte *InData, const USize InSize ) noexcept
    {
        UInt64 Hash = InSeed;
        for ( USize Index = 0ULL; Index < InSize; ++Index )
        {
            Hash ^= static_cast<UInt64>( InData[Index] );
            Hash *= FNVPrime;
        }
        return Hash;
    }

} // namespace
} // namespace Lumen

inline void Lumen::FReplicationRecord::SetLocalTransform ( const Maths::FMat4x4f &InTransform ) noexcept
{
    static_assert( sizeof( Maths::FMat4x4f ) <= sizeof( Payload ) );
    std::memcpy( Payload, &InTransform, sizeof( Maths::FMat4x4f ) );
    PayloadSize = static_cast<UInt16>( sizeof( Maths::FMat4x4f ) );
}

inline Lumen::Maths::FMat4x4f Lumen::FReplicationRecord::GetLocalTransform () const noexcept
{
    Maths::FMat4x4f Transform;
    if ( PayloadSize >= sizeof( Maths::FMat4x4f ) )
    {
        std::memcpy( &Transform, Payload, sizeof( Maths::FMat4x4f ) );
    }
    return Transform;
}

inline void Lumen::FReplicationRecord::SetParent ( const FPersistentActorId InParent ) noexcept
{
    WriteU64( Payload, InParent.GetValue() );
    PayloadSize = 8U;
}

inline Lumen::FPersistentActorId Lumen::FReplicationRecord::GetParent () const noexcept
{
    return FPersistentActorId( PayloadSize >= 8U ? ReadU64( Payload ) : 0ULL );
}

inline void Lumen::FReplicationRecord::SetVisibility ( const Bool bInVisible ) noexcept
{
    Payload[0U] = bInVisible ? static_cast<Byte>( 1U ) : static_cast<Byte>( 0U );
    PayloadSize = 1U;
}

inline Lumen::Bool Lumen::FReplicationRecord::GetVisibility () const noexcept
{
    return PayloadSize >= 1U and Payload[0U] != static_cast<Byte>( 0U );
}

inline void Lumen::FReplicationRecord::SetControl ( const FReplicationControlPayload &InControl ) noexcept
{
    std::memcpy( Payload, &InControl, sizeof( FReplicationControlPayload ) );
    PayloadSize = static_cast<UInt16>( sizeof( FReplicationControlPayload ) );
}

inline Lumen::FReplicationControlPayload Lumen::FReplicationRecord::GetControl () const noexcept
{
    FReplicationControlPayload Control;
    if ( PayloadSize >= sizeof( FReplicationControlPayload ) )
    {
        std::memcpy( &Control, Payload, sizeof( FReplicationControlPayload ) );
    }
    return Control;
}

inline void Lumen::FReplicationRecord::SetInput ( const UInt64 InInputSequence, const Byte *InData, const UInt16 InSize ) noexcept
{
    WriteU64( Payload, InInputSequence );
    WriteU16( Payload + 8U, InSize );
    if ( InSize > 0U and InData != nullptr )
    {
        std::memcpy( Payload + 10U, InData, InSize );
    }
    PayloadSize = static_cast<UInt16>( 10U + InSize );
}

inline Lumen::UInt64 Lumen::FReplicationRecord::GetInputSequence () const noexcept
{
    return PayloadSize >= 8U ? ReadU64( Payload ) : 0ULL;
}

inline Lumen::TSpan<const Lumen::Byte> Lumen::FReplicationRecord::GetInputPayload () const noexcept
{
    if ( PayloadSize < 10U )
    {
        return {};
    }
    const UInt16 Size = ReadU16( Payload + 8U );
    if ( Size > MaxReplicationInputPayloadBytes or static_cast<USize>( 10U ) + Size > PayloadSize )
    {
        return {};
    }
    return { Payload + 10U, Size };
}

inline void Lumen::FReplicationFrameBuilder::Reset () noexcept
{
    Buffer.clear();
    Records.clear();
}

inline Lumen::TExpected<void, Lumen::EReplicationError>
Lumen::FReplicationFrameBuilder::TryAppend ( const FReplicationRecord &InRecord ) noexcept
{
    if ( Records.size() >= Records.capacity() )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
    }
    Records.push_back( InRecord );
    return {};
}

inline Lumen::TSpan<const Lumen::FReplicationRecord> Lumen::FReplicationFrameBuilder::GetRecords () const noexcept
{
    return { Records.data(), Records.size() };
}

inline Lumen::USize Lumen::FReplicationFrameBuilder::GetByteCapacity () const noexcept
{
    return Buffer.capacity();
}

inline Lumen::USize Lumen::FReplicationFrameBuilder::GetRecordCapacity () const noexcept
{
    return Records.capacity();
}

inline Lumen::TExpected<Lumen::USize, Lumen::EReplicationError>
Lumen::FReplicationFrameBuilder::TryWrite ( TSpan<Byte> OutBytes, const FReplicationFrameHeader &InHeader ) const noexcept
{
    const USize RecordCount = Records.size();
    const USize Total =
        ReplicationFrameHeaderBytes + ( RecordCount * ReplicationFrameRecordBytes ) + ReplicationFrameChecksumBytes;
    if ( OutBytes.size() < Total or RecordCount > 0xFFFFULL )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
    }

    Byte *Destination = OutBytes.data();
    for ( USize Index = 0ULL; Index < ReplicationFrameHeaderBytes; ++Index )
    {
        Destination[Index] = static_cast<Byte>( 0U );
    }
    WriteU64( Destination + 0U, InHeader.Sequence );
    WriteU64( Destination + 8U, InHeader.AckedSequence );
    WriteU64( Destination + 16U, InHeader.BaselineSequence );
    WriteU64( Destination + 24U, InHeader.FrameId );
    Destination[32U] = static_cast<Byte>( InHeader.Direction );
    Destination[33U] = static_cast<Byte>( InHeader.Channel );
    WriteU16( Destination + 34U, static_cast<UInt16>( RecordCount ) );
    WriteU16( Destination + 36U, InHeader.Flags );
    WriteU32( Destination + 40U, static_cast<UInt32>( Total ) );

    for ( USize RecordIndex = 0ULL; RecordIndex < RecordCount; ++RecordIndex )
    {
        const FReplicationRecord &Record = Records[RecordIndex];
        Byte *RecordBytes = Destination + ReplicationFrameHeaderBytes + ( RecordIndex * ReplicationFrameRecordBytes );
        WriteU64( RecordBytes, Record.Actor.GetValue() );
        RecordBytes[8U] = static_cast<Byte>( Record.Kind );
        RecordBytes[9U] = Record.PropertyRank;
        WriteU16( RecordBytes + 10U, Record.PayloadSize );
        std::memcpy( RecordBytes + 12U, Record.Payload, sizeof( Record.Payload ) );
    }

    const UInt64 Checksum = HashBytes( FNVOffsetBasis, Destination, ReplicationFrameHeaderBytes - ReplicationFrameChecksumBytes );
    const UInt64 WithRecords =
        HashBytes( Checksum, Destination + ReplicationFrameHeaderBytes, RecordCount * ReplicationFrameRecordBytes );
    WriteU64( Destination + 48U, WithRecords );
    return Total;
}

inline Lumen::TExpected<Lumen::FReplicationFrame, Lumen::EReplicationError>
Lumen::FReplicationFrameBuilder::TryParse ( TSpan<const Byte> InBytes ) noexcept
{
    if ( InBytes.size() < ReplicationFrameHeaderBytes + ReplicationFrameChecksumBytes )
    {
        return TUnexpected<EReplicationError>( EReplicationError::MalformedFrame );
    }
    const Byte *Source          = InBytes.data();
    const UInt32 DeclaredLength = ReadU32( Source + 40U );
    if ( DeclaredLength < ReplicationFrameHeaderBytes + ReplicationFrameChecksumBytes or DeclaredLength > InBytes.size() )
    {
        return TUnexpected<EReplicationError>( EReplicationError::MalformedFrame );
    }
    const UInt16 RecordCount   = ReadU16( Source + 34U );
    const USize ExpectedLength = ReplicationFrameHeaderBytes +
                                 ( static_cast<USize>( RecordCount ) * ReplicationFrameRecordBytes ) +
                                 ReplicationFrameChecksumBytes;
    if ( static_cast<USize>( DeclaredLength ) != ExpectedLength or RecordCount > Records.capacity() )
    {
        return TUnexpected<EReplicationError>( EReplicationError::MalformedFrame );
    }
    if ( Source[32U] > static_cast<Byte>( 1U ) or Source[33U] > static_cast<Byte>( 1U ) )
    {
        return TUnexpected<EReplicationError>( EReplicationError::MalformedFrame );
    }

    const UInt64 StoredChecksum = ReadU64( Source + 48U );
    const UInt64 HeaderChecksum =
        HashBytes( FNVOffsetBasis, Source, ReplicationFrameHeaderBytes - ReplicationFrameChecksumBytes );
    const UInt64 Computed = HashBytes( HeaderChecksum, Source + ReplicationFrameHeaderBytes,
                                       static_cast<USize>( RecordCount ) * ReplicationFrameRecordBytes );
    if ( Computed != StoredChecksum )
    {
        return TUnexpected<EReplicationError>( EReplicationError::MalformedFrame );
    }

    Records.clear();
    for ( USize RecordIndex = 0ULL; RecordIndex < static_cast<USize>( RecordCount ); ++RecordIndex )
    {
        const Byte *RecordBytes = Source + ReplicationFrameHeaderBytes + ( RecordIndex * ReplicationFrameRecordBytes );
        FReplicationRecord Record;
        Record.Actor        = FPersistentActorId( ReadU64( RecordBytes ) );
        Record.Kind         = static_cast<EActorReplicationKind>( RecordBytes[8U] );
        Record.PropertyRank = RecordBytes[9U];
        Record.PayloadSize  = ReadU16( RecordBytes + 10U );
        std::memcpy( Record.Payload, RecordBytes + 12U, sizeof( Record.Payload ) );
        Records.push_back( Record );
    }

    FReplicationFrame Frame;
    Frame.Header.Sequence         = ReadU64( Source + 0U );
    Frame.Header.AckedSequence    = ReadU64( Source + 8U );
    Frame.Header.BaselineSequence = ReadU64( Source + 16U );
    Frame.Header.FrameId          = ReadU64( Source + 24U );
    Frame.Header.Direction        = static_cast<EReplicationDirection>( Source[32U] );
    Frame.Header.Channel          = static_cast<EReplicationChannel>( Source[33U] );
    Frame.Header.RecordCount      = RecordCount;
    Frame.Header.Flags            = ReadU16( Source + 36U );
    Frame.Header.ByteLength       = DeclaredLength;
    Frame.Records                 = GetRecords();
    return Frame;
}
