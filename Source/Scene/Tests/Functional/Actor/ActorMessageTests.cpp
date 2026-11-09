/*
 * @file ActorMessageTests.cpp
 * @brief Functional tests for FMessage.
 */

#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>
#include <Concepts/TriviallyCopyable.hpp>
#include <Messages/EngineMessageTypes.hpp>
#include <Tests/UnitTest.hpp>

#include <cstring>
#include <type_traits>

namespace Lumen
{

namespace
{

    /** @brief Minimal payload used for generic round trips. */
    struct FBooleanPayload
    {
        Bool bFlag  = false;
        Int32 Value = 0;
    };

    /** @brief Padding free payload, guaranteeing a bitexact memory image. */
    struct FTextPayload
    {
        Byte Prefix[3] = { 'L', 'U', 'M' };
        Byte Suffix[5] = { 'E', 'N', 'I', 'G', 'M' };
    };

    /** @brief Payload filling the whole inline buffer, one byte per index. */
    struct FFullBufferPayload
    {
        Byte Bytes[112] = {};
    };

    /** @brief Payload with the strictest alignment that the buffer offset still satisfies. */
    struct FAligned16Payload
    {
        alignas( 16 ) UInt64 Value = 0ULL;
    };

    /** @brief Payload one byte too large for the inline buffer. */
    struct FOversizedPayload
    {
        Byte Bytes[113] = {};
    };

    /** @brief Payload carrying ownership, rejected by CTriviallyCopyablePayload. */
    struct FNonTrivialPayload
    {
        UInt64 Value = 0ULL;
        ~FNonTrivialPayload ()
        {
        }
    };

    /**
     * @concept CViableMessagePayload
     * @brief True when FMessage::Make accepts the payload type.
     */
    template <typename PayloadType>
    concept CViableMessagePayload =
        requires( UInt32 InType, ActorID InSender, PayloadType InPayload ) { FMessage::Make( InType, InSender, InPayload ); };

    constexpr FFullBufferPayload MakeRampPayload ( const Byte InOffset ) noexcept
    {
        FFullBufferPayload Payload;

        for ( USize Index = 0ULL; Index < sizeof( Payload.Bytes ); ++Index )
        {
            Payload.Bytes[Index] = static_cast<Byte>( ( Index + static_cast<USize>( InOffset ) ) & 0xFFULL );
        }

        return Payload;
    }

    constexpr Bool MatchesRamp ( const Byte *InBytes, const Byte InOffset ) noexcept
    {
        for ( USize Index = 0ULL; Index < 112ULL; ++Index )
        {
            if ( InBytes[Index] != static_cast<Byte>( ( Index + static_cast<USize>( InOffset ) ) & 0xFFULL ) )
            {
                return false;
            }
        }

        return true;
    }

} // namespace

/**
 * Static invariants
 */

LUMEN_TEST( FMessage_Layout, MatchesCacheLineContract )
{
    static_assert( sizeof( FMessage ) == 128, "FMessage should be exactly 128 bytes for performance reasons." );
    static_assert( alignof( FMessage ) == 64, "FMessage must be aligned to a whole CPU cache line." );
    static_assert( FMessage::MaxPayloadSize == 112, "The inline buffer must be 112 bytes." );
    static_assert( std::is_trivially_copyable_v<FMessage>, "FMessage travels lock-free and must stay trivially copyable." );

    LUMEN_CHECK_EQ( sizeof( FMessage ), 128ULL );
    LUMEN_CHECK_EQ( alignof( FMessage ), 64ULL );
    LUMEN_CHECK_EQ( FMessage::MaxPayloadSize, 112ULL );
}

LUMEN_TEST( FMessage_Layout, HeaderAvoidsPaddingHoles )
{
    /**
     * INFO: Sender (8), Type (4), PayloadSize (2) and Flags (2) must pack tightly
     *       so the 16 bytes of header leave the whole rest to the 112 bytes
     *       inline payload starting on a 16 bytes boundary.
     */
    constexpr USize HeaderSize = sizeof( ActorID ) + sizeof( UInt32 ) + sizeof( UInt16 ) + sizeof( UInt16 );

    static_assert( HeaderSize == 16ULL );
    static_assert( HeaderSize + sizeof( FFullBufferPayload ) == sizeof( FMessage ) );

    LUMEN_CHECK_EQ( HeaderSize, 16ULL );
}

LUMEN_TEST( FMessage_Layout, PayloadDataStartsAfterHeader )
{
    const FMessage Msg       = FMessage::Make( 1U, 1ULL, FBooleanPayload{ .bFlag = true, .Value = 7 } );
    const UPtr PayloadOffset = reinterpret_cast<UPtr>( Msg.PayloadData ) - reinterpret_cast<UPtr>( &Msg );
    LUMEN_CHECK_EQ( PayloadOffset, 16ULL );

    const FBooleanPayload Payload = Msg.GetPayload<FBooleanPayload>();
    LUMEN_CHECK_TRUE( Payload.bFlag );
    LUMEN_CHECK_EQ( Payload.Value, 7 );
}

/**
 * Default construction
 */

LUMEN_TEST( FMessage_Construction, DefaultMessageIsEmpty )
{
    constexpr FMessage Msg;

    static_assert( Msg.Sender == 0ULL );
    static_assert( Msg.Type == 0U );
    static_assert( Msg.PayloadSize == 0U );
    static_assert( Msg.Flags == 0U );

    LUMEN_CHECK_EQ( Msg.Sender, 0ULL );
    LUMEN_CHECK_EQ( Msg.Type, 0U );
    LUMEN_CHECK_EQ( Msg.PayloadSize, 0U );
    LUMEN_CHECK_EQ( Msg.Flags, 0U );

    constexpr Byte ZeroedBuffer[FMessage::MaxPayloadSize] = {};

    LUMEN_CHECK_TRUE( std::memcmp( Msg.PayloadData, ZeroedBuffer, FMessage::MaxPayloadSize ) == 0 );
}

LUMEN_TEST( FMessage_Construction, PayloadReadsBackAsZeroWithoutMake )
{
    /**
     * INFO: A default message body is zeroed, so reading the inline buffer
     *       through GetPayload yields the payload type's zero state.
     */
    constexpr FMessage Msg;

    const auto &Payload = Msg.GetPayload<FTickPayload>();

    LUMEN_CHECK_TRUE( Payload.DeltaTime == 0.0 );
}

LUMEN_TEST( FMessage_Construction, PayloadBufferStartsFullyZeroed )
{
    constexpr FMessage Msg;

    for ( USize Index = 0ULL; Index < FMessage::MaxPayloadSize; ++Index )
    {
        LUMEN_CHECK_TRUE( Msg.PayloadData[Index] == 0 );
    }
}

/**
 * Make without payload
 */

LUMEN_TEST( FMessage_Make, HeaderOnlyKeepsPayloadZeroed )
{
    constexpr ActorID SenderId   = 777ULL;
    constexpr UInt32 MessageType = 33U;

    const FMessage Msg = FMessage::Make( MessageType, SenderId );

    LUMEN_CHECK_EQ( Msg.Sender, SenderId );
    LUMEN_CHECK_EQ( Msg.Type, MessageType );

    /**
     * INFO: Header-only Make documents PayloadSize as 0 — the absence of a
     *       payload is now observable instead of assumed.
     */
    LUMEN_CHECK_EQ( Msg.PayloadSize, 0U );

    constexpr Byte ZeroedBuffer[FMessage::MaxPayloadSize] = {};
    LUMEN_CHECK_TRUE( std::memcmp( Msg.PayloadData, ZeroedBuffer, FMessage::MaxPayloadSize ) == 0 );
}

LUMEN_TEST( FMessage_Make, ZeroValuesProduceZeroMessage )
{
    const FMessage Msg = FMessage::Make( 0U, 0ULL );

    LUMEN_CHECK_EQ( Msg.Sender, 0ULL );
    LUMEN_CHECK_EQ( Msg.Type, 0U );
}

/**
 * Make with payload
 */

LUMEN_TEST( FMessage_Make, PayloadRoundTrips )
{
    constexpr ActorID SenderId   = 1234ULL;
    constexpr UInt32 MessageType = 42U;

    constexpr FBooleanPayload SourcePayload{ .bFlag = true, .Value = -9 };

    const FMessage Msg = FMessage::Make( MessageType, SenderId, SourcePayload );

    LUMEN_CHECK_EQ( Msg.Sender, SenderId );
    LUMEN_CHECK_EQ( Msg.Type, MessageType );

    /**
     * INFO: Make<T> documents PayloadSize as the payload size — the new header
     *       contract must be observable from the produced message.
     */
    LUMEN_CHECK_EQ( Msg.PayloadSize, static_cast<UInt16>( sizeof( FBooleanPayload ) ) );

    const FBooleanPayload &Payload = Msg.GetPayload<FBooleanPayload>();

    LUMEN_CHECK_TRUE( Payload.bFlag );
    LUMEN_CHECK_EQ( Payload.Value, -9 );
}

LUMEN_TEST( FMessage_Make, PayloadIsBitwiseCopied )
{
    /**
     * INFO: The inline buffer must hold an exact bitwise image of the payload,
     *       independently readable through GetPayload.
     */
    constexpr FFullBufferPayload SourcePayload    = MakeRampPayload( 1U );
    constexpr FFullBufferPayload ReferencePayload = MakeRampPayload( 1U );

    const FMessage Msg = FMessage::Make( 5U, 9ULL, SourcePayload );

    static_assert( MatchesRamp( SourcePayload.Bytes, 1U ) );
    static_assert( MatchesRamp( ReferencePayload.Bytes, 1U ) );

    LUMEN_CHECK_TRUE( MatchesRamp( Msg.PayloadData, 1U ) );
    LUMEN_CHECK_TRUE( std::memcmp( Msg.PayloadData, ReferencePayload.Bytes, FMessage::MaxPayloadSize ) == 0 );
}

LUMEN_TEST( FMessage_Make, PayloadTailStaysZeroed )
{
    /**
     * INFO: Writing a payload smaller than the buffer must not dirty the
     *       remaining inline bytes.
     */
    constexpr FBooleanPayload SourcePayload{ .bFlag = true, .Value = 77 };

    const FMessage Msg = FMessage::Make( 1U, 1ULL, SourcePayload );

    constexpr USize WrittenBytes = sizeof( SourcePayload );

    for ( USize Index = WrittenBytes; Index < FMessage::MaxPayloadSize; ++Index )
    {
        LUMEN_CHECK_TRUE( Msg.PayloadData[Index] == 0 );
    }
}

LUMEN_TEST( FMessage_Make, PayloadImageMatchesSourceWhenBound )
{
    /**
     * INFO: A padding free payload maps onto the buffer contiguously.
     */
    constexpr FTextPayload SourcePayload;

    const FMessage Msg = FMessage::Make( 2U, 2ULL, SourcePayload );

    for ( USize Index = 0ULL; Index < 3ULL; ++Index )
    {
        LUMEN_CHECK_TRUE( Msg.PayloadData[Index] == SourcePayload.Prefix[Index] );
    }

    for ( USize Index = 0ULL; Index < 5ULL; ++Index )
    {
        LUMEN_CHECK_TRUE( Msg.PayloadData[3ULL + Index] == SourcePayload.Suffix[Index] );
    }
}

LUMEN_TEST( FMessage_Make, PayloadKeepsStricterAlignmentUsable )
{
    /**
     * INFO: The whole message is 64 bytes aligned, so a 16 bytes aligned payload
     *       starting at offset 16 stays properly aligned for GetPayload.
     */
    constexpr FAligned16Payload SourcePayload{ .Value = 0xDEADBEEFULL };

    const FMessage Msg = FMessage::Make( 9U, 8ULL, SourcePayload );

    const FAligned16Payload &Payload = Msg.GetPayload<FAligned16Payload>();

    LUMEN_CHECK_EQ( Payload.Value, 0xDEADBEEFULL );
}

LUMEN_TEST( FMessage_Payload, GetPayloadReturnsIndependentCopies )
{
    /**
     * INFO: The former thread_local fallback aliased every same-type GetPayload
     *       call on a thread to one object. Two calls must now yield values with
     *       independent storage.
     */
    const FMessage Msg = FMessage::Make( 1U, 0ULL, FBooleanPayload{ .bFlag = true, .Value = 5 } );

    FBooleanPayload First  = Msg.GetPayload<FBooleanPayload>();
    FBooleanPayload Second = Msg.GetPayload<FBooleanPayload>();

    LUMEN_CHECK_EQ( First.Value, 5 );
    LUMEN_CHECK_EQ( Second.Value, 5 );

    First.Value = 999;

    LUMEN_CHECK_EQ( Second.Value, 5 );
    LUMEN_CHECK_EQ( Msg.GetPayload<FBooleanPayload>().Value, 5 );

    /** INFO: The returned object is a copy, not an alias into PayloadData. */
    LUMEN_CHECK_FALSE( reinterpret_cast<const void *>( &Second ) == reinterpret_cast<const void *>( Msg.PayloadData ) );
}

/**
 * Engine payloads
 */

LUMEN_TEST( FMessage_Make, EnginePayloadsFitInlineBuffer )
{
    static_assert( sizeof( FTickPayload ) <= FMessage::MaxPayloadSize );
    static_assert( sizeof( FTransformPayload ) <= FMessage::MaxPayloadSize );
    static_assert( sizeof( FInputAxisPayload ) <= FMessage::MaxPayloadSize );
    static_assert( sizeof( FSubmitDrawPayload ) <= FMessage::MaxPayloadSize );
    static_assert( sizeof( FSetCameraTargetPayload ) <= FMessage::MaxPayloadSize );

    static_assert( Concepts::CTriviallyCopyablePayload<FTickPayload> );
    static_assert( Concepts::CTriviallyCopyablePayload<FTransformPayload> );
    static_assert( Concepts::CTriviallyCopyablePayload<FInputAxisPayload> );
    static_assert( Concepts::CTriviallyCopyablePayload<FSubmitDrawPayload> );
    static_assert( Concepts::CTriviallyCopyablePayload<FSetCameraTargetPayload> );

    const UInt32 TickType = static_cast<UInt32>( EEngineMessage::Tick );

    const FTickPayload SourcePayload{ .DeltaTime = 16.0 };
    const FMessage TickMessage = FMessage::Make( TickType, 0ULL, SourcePayload );

    LUMEN_CHECK_EQ( TickMessage.Type, TickType );
    LUMEN_CHECK_EQ( TickMessage.GetPayload<FTickPayload>().DeltaTime, 16.0 );
}

LUMEN_TEST( FMessage_Make, EngineTransformPayloadRoundTripsExactly )
{
    constexpr UInt32 TransformUpdateType = static_cast<UInt32>( EEngineMessage::TransformUpdate );

    const Maths::FMatrix4x4f SourceTransform = Maths::FMatrix4x4f::Translate( Maths::FVec3f{ 1.0F, 2.0F, 3.0F } );
    const FTransformPayload SourcePayload{ .NewTransform = SourceTransform };

    const FMessage TransformMessage  = FMessage::Make( TransformUpdateType, 55ULL, SourcePayload );
    const FTransformPayload &Payload = TransformMessage.GetPayload<FTransformPayload>();

    LUMEN_CHECK_EQ( TransformMessage.Type, TransformUpdateType );
    LUMEN_CHECK_EQ( TransformMessage.Sender, 55ULL );
    LUMEN_CHECK_TRUE( Payload.NewTransform == SourceTransform );
}

LUMEN_TEST( FMessage_Make, EngineInputAxisPayloadRoundTrips )
{
    constexpr UInt32 InputAxisType = static_cast<UInt32>( EEngineMessage::InputAxis );

    const FInputAxisPayload SourcePayload{ .AxisId = 8U, .Value = -0.5F };
    const FMessage AxisMessage = FMessage::Make( InputAxisType, 66ULL, SourcePayload );

    const FInputAxisPayload &Payload = AxisMessage.GetPayload<FInputAxisPayload>();

    LUMEN_CHECK_EQ( Payload.AxisId, 8U );
    LUMEN_CHECK_TRUE( Payload.Value == -0.5F );
}

/**
 * Payload contract
 */

LUMEN_TEST( FMessage_Contract, OnlyTriviallyCopyablePayloadsFit )
{
    static_assert( Concepts::CTriviallyCopyablePayload<FBooleanPayload> );
    static_assert( not Concepts::CTriviallyCopyablePayload<FNonTrivialPayload> );

    LUMEN_CHECK_TRUE( CViableMessagePayload<FBooleanPayload> );
    LUMEN_CHECK_FALSE( CViableMessagePayload<FNonTrivialPayload> );
}

LUMEN_TEST( FMessage_Contract, PayloadMustNotExceedTheInlineBuffer )
{
    static_assert( Concepts::CTriviallyCopyablePayload<FOversizedPayload> );
    static_assert( sizeof( FOversizedPayload ) == FMessage::MaxPayloadSize + 1ULL );

    LUMEN_CHECK_TRUE( CViableMessagePayload<FFullBufferPayload> );
    LUMEN_CHECK_FALSE( CViableMessagePayload<FOversizedPayload> );
}

/**
 * Copy semantics
 */

LUMEN_TEST( FMessage_Copy, FieldImagesMatch )
{
    constexpr FBooleanPayload SourcePayload{ .bFlag = false, .Value = -13 };

    const FMessage Built    = FMessage::Make( 3U, 3ULL, SourcePayload );
    const FMessage Copy     = Built;
    const FMessage Assigned = Copy;

    LUMEN_CHECK_EQ( Copy.Sender, Built.Sender );
    LUMEN_CHECK_EQ( Copy.Type, Built.Type );
    LUMEN_CHECK_EQ( Assigned.Sender, Built.Sender );
    LUMEN_CHECK_EQ( Assigned.Type, Built.Type );
    LUMEN_CHECK_TRUE( std::memcmp( Assigned.PayloadData, Built.PayloadData, FMessage::MaxPayloadSize ) == 0 );
}

LUMEN_TEST( FMessage_Copy, CopyIsIndependentlyMutable )
{
    FBooleanPayload SourcePayload{ .bFlag = false, .Value = 21 };

    FMessage Source = FMessage::Make( 4U, 4ULL, SourcePayload );
    FMessage Copy   = Source;

    Source.Sender = 1ULL;
    Source.Type   = 31U;

    LUMEN_CHECK_EQ( Copy.Sender, 4ULL );
    LUMEN_CHECK_EQ( Copy.Type, 4U );
    LUMEN_CHECK_TRUE( Copy.GetPayload<FBooleanPayload>().bFlag == false );
    LUMEN_CHECK_EQ( Copy.GetPayload<FBooleanPayload>().Value, 21 );

    LUMEN_CHECK_EQ( Source.Sender, 1ULL );
    LUMEN_CHECK_EQ( Source.Type, 31U );
}

} // namespace Lumen
