/// @file ActorMessageTests.cpp
/// @brief Unit tests for actor message payload storage and metadata.

#include <Actor/ActorMessage.hpp>
#include <Tests/UnitTest.hpp>

#include <array>
#include <concepts>
#include <type_traits>

namespace Lumen
{

template <USize Size, USize Alignment = 1ULL> struct alignas( Alignment ) TTestPayload
{
    std::array<Byte, Size> Data = {};

    Bool operator==( const TTestPayload & ) const = default;
};

template <USize Size, USize Alignment> struct TMessageTraits<TTestPayload<Size, Alignment>>
{
    static constexpr Bool bEnabled  = true;
    static constexpr UInt32 Type    = 1000U + static_cast<UInt32>( Size ) + static_cast<UInt32>( Alignment );
    static constexpr UInt16 Version = 4U;
};

struct FOtherTestPayload
{
    std::array<Byte, 8ULL> Data = {};

    Bool operator==( const FOtherTestPayload & ) const = default;
};

template <> struct TMessageTraits<FOtherTestPayload>
{
    static constexpr Bool bEnabled  = true;
    static constexpr UInt32 Type    = 2000U;
    static constexpr UInt16 Version = 4U;
};

struct FNewerTestPayload
{
    std::array<Byte, 8ULL> Data = {};

    Bool operator==( const FNewerTestPayload & ) const = default;
};

template <> struct TMessageTraits<FNewerTestPayload>
{
    static constexpr Bool bEnabled  = true;
    static constexpr UInt32 Type    = 1009U;
    static constexpr UInt16 Version = 5U;
};

struct FUnregisteredPayload
{
    UInt32 Value = 0U;
};

static_assert( Concepts::CMessagePayload<TTestPayload<16ULL>> );
static_assert( not Concepts::CMessagePayload<FUnregisteredPayload> );
static_assert( not Concepts::CMessagePayload<const void *> );
static_assert( std::same_as<decltype( std::declval<const FMessage &>().GetPayload<TTestPayload<8ULL>>() ), TTestPayload<8ULL>> );
static_assert( sizeof( FMessage ) == 128ULL );
static_assert( offsetof( FMessage, PayloadData ) == 16ULL );

namespace
{

    template <typename PayloadType> void CheckRoundTrip ( const PayloadType &InPayload )
    {
        constexpr UInt32 Type =
            3000U + static_cast<UInt32>( sizeof( PayloadType ) ) + static_cast<UInt32>( alignof( PayloadType ) );
        const FMessage Message = FMessage::Make( Type, ActorID{ 12U, 3U }, InPayload );

        LUMEN_CHECK_EQ( Message.Type, Type );
        LUMEN_CHECK_EQ( Message.PayloadSize, sizeof( PayloadType ) );
        LUMEN_CHECK_EQ( Message.Version, 0U );
        LUMEN_CHECK_EQ( Message.GetPayload<PayloadType>( Type ), InPayload );

        const TOptional<PayloadType> Result = Message.TryGetPayload<PayloadType>( Type );
        LUMEN_CHECK_HAS_VALUE( Result );
        LUMEN_CHECK_EQ( *Result, InPayload );
    }

    template <USize Size> TTestPayload<Size> MakePayload ()
    {
        TTestPayload<Size> Payload;
        for ( USize Index = 0ULL; Index < Size; ++Index )
        {
            Payload.Data[Index] = static_cast<Byte>( ( Index * 37ULL ) & 0xFFULL );
        }
        return Payload;
    }

    template <USize Size, USize Alignment> TTestPayload<Size, Alignment> MakeAlignedPayload ()
    {
        TTestPayload<Size, Alignment> Payload;
        for ( USize Index = 0ULL; Index < Size; ++Index )
        {
            Payload.Data[Index] = static_cast<Byte>( ( Index * 53ULL ) & 0xFFULL );
        }
        return Payload;
    }

} // namespace

LUMEN_TEST( ActorMessage_Payload, OneByte )
{
    CheckRoundTrip( MakePayload<1ULL>() );
}

LUMEN_TEST( ActorMessage_Payload, EightBytes )
{
    CheckRoundTrip( MakePayload<8ULL>() );
}

LUMEN_TEST( ActorMessage_Payload, SixteenBytes )
{
    CheckRoundTrip( MakePayload<16ULL>() );
}

LUMEN_TEST( ActorMessage_Payload, MaximumSize )
{
    CheckRoundTrip( MakePayload<112ULL>() );
}

LUMEN_TEST( ActorMessage_Payload, OverAlignedTypes )
{
    CheckRoundTrip( MakeAlignedPayload<16ULL, 16ULL>() );
    CheckRoundTrip( MakeAlignedPayload<16ULL, 32ULL>() );
    CheckRoundTrip( MakeAlignedPayload<16ULL, 64ULL>() );
}

LUMEN_TEST( ActorMessage_Payload, SizeMismatchIsRecoverable )
{
    using ExpectedPayload  = TTestPayload<16ULL>;
    const FMessage Message = FMessage::Make( TMessageTraits<ExpectedPayload>::Type, TMessageTraits<ExpectedPayload>::Version,
                                             ActorID{}, MakePayload<8ULL>() );
    const TOptional<ExpectedPayload> Result = Message.TryGetPayload<ExpectedPayload>();

    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( ActorMessage_Payload, SameSizeWithDifferentTypeIsRejected )
{
    const FMessage Message = FMessage::Make( ActorID{}, MakePayload<8ULL>() );

    LUMEN_CHECK_FALSE( Message.TryGetPayload<FOtherTestPayload>().HasValue() );
    LUMEN_CHECK_HAS_VALUE( Message.TryGetPayload<TTestPayload<8ULL>>() );
}

LUMEN_TEST( ActorMessage_Payload, SameSizeWithDifferentVersionIsRejected )
{
    const FMessage Message = FMessage::Make( ActorID{}, MakePayload<8ULL>() );

    LUMEN_CHECK_FALSE( Message.TryGetPayload<FNewerTestPayload>().HasValue() );
    LUMEN_CHECK_HAS_VALUE( Message.TryGetPayload<TTestPayload<8ULL>>() );
}

LUMEN_TEST( ActorMessage_Payload, ManualAndTraitIdentitiesAreDistinct )
{
    const auto Payload     = MakePayload<8ULL>();
    const FMessage Message = FMessage::Make( 91U, 7U, ActorID{}, Payload );

    LUMEN_CHECK_EQ( Message.Type, 91U );
    LUMEN_CHECK_EQ( Message.Version, 7U );
    LUMEN_CHECK_FALSE( Message.TryGetPayload<TTestPayload<8ULL>>().HasValue() );
    LUMEN_CHECK_FALSE( Message.TryGetPayload<TTestPayload<8ULL>>( 91U, 6U ).HasValue() );
    LUMEN_CHECK_FALSE( Message.TryGetPayload<TTestPayload<8ULL>>( 1009U, 7U ).HasValue() );

    const TOptional<TTestPayload<8ULL>> Result = Message.TryGetPayload<TTestPayload<8ULL>>( 91U, 7U );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( *Result, Payload );
}

LUMEN_TEST( ActorMessage_Payload, TraitsProvideTypeAndVersion )
{
    const auto Payload     = MakePayload<8ULL>();
    const FMessage Message = FMessage::Make( ActorID{ 4U, 2U }, Payload );

    LUMEN_CHECK_EQ( Message.Type, 1009U );
    LUMEN_CHECK_EQ( Message.Version, 4U );
    LUMEN_CHECK_EQ( Message.Sender, ( ActorID{ 4U, 2U } ) );
    LUMEN_CHECK_EQ( Message.GetPayload<std::remove_cv_t<decltype( Payload )>>(), Payload );
}

LUMEN_TEST( ActorMessage_Payload, EmptyPayloadHasZeroSize )
{
    const FMessage Message = FMessage::Make( 12U, ActorID{} );

    LUMEN_CHECK_EQ( Message.PayloadSize, 0U );
    LUMEN_CHECK_EQ( Message.Version, 0U );
    LUMEN_CHECK_FALSE( Message.TryGetPayload<TTestPayload<1ULL>>().HasValue() );
}

} // namespace Lumen
