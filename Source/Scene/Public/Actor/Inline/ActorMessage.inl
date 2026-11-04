/// @file ActorMessage.inl
/// @brief Inline implementation for ActorMessage in Lumen Engine

#pragma once

#include <Actor/ActorMessage.hpp>

#include <array>
#include <bit>
#include <cassert>
#include <cstdlib>
#include <cstring>

template <Lumen::Concepts::CMessagePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] Lumen::FMessage Lumen::FMessage::Make ( UInt32 InType, ActorID InSender, const PayloadType &InPayload ) noexcept
{
    return Make( InType, 0U, InSender, InPayload );
}

template <Lumen::Concepts::CMessagePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] Lumen::FMessage
Lumen::FMessage::Make ( UInt32 InType, UInt16 InVersion, ActorID InSender, const PayloadType &InPayload ) noexcept
{
    FMessage Message;
    Message.Type        = InType;
    Message.Sender      = InSender;
    Message.PayloadSize = static_cast<UInt16>( sizeof( PayloadType ) );
    Message.Version     = InVersion;

    std::memcpy( Message.PayloadData, &InPayload, sizeof( PayloadType ) );
    return Message;
}

template <Lumen::Concepts::CIdentifiedMessagePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] Lumen::FMessage Lumen::FMessage::Make ( ActorID InSender, const PayloadType &InPayload ) noexcept
{
    return Make( static_cast<UInt32>( TMessageTraits<PayloadType>::Type ),
                 static_cast<UInt16>( TMessageTraits<PayloadType>::Version ), InSender, InPayload );
}

[[nodiscard]] inline Lumen::FMessage Lumen::FMessage::Make ( UInt32 InType, ActorID InSender ) noexcept
{
    FMessage Message;
    Message.Type   = InType;
    Message.Sender = InSender;

    return Message;
}

template <Lumen::Concepts::CIdentifiedMessagePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] inline PayloadType Lumen::FMessage::GetPayload () const noexcept
{
    return GetPayload<PayloadType>( static_cast<UInt32>( TMessageTraits<PayloadType>::Type ),
                                    static_cast<UInt16>( TMessageTraits<PayloadType>::Version ) );
}

template <Lumen::Concepts::CMessagePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] inline PayloadType Lumen::FMessage::GetPayload ( const UInt32 InType, const UInt16 InVersion ) const noexcept
{
    TOptional<PayloadType> Result = TryGetPayload<PayloadType>( InType, InVersion );

    assert( Result.HasValue() );
    if ( not Result.HasValue() )
    {
        std::abort();
    }

    return *Result;
}

template <Lumen::Concepts::CIdentifiedMessagePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] inline Lumen::TOptional<PayloadType> Lumen::FMessage::TryGetPayload () const noexcept
{
    return TryGetPayload<PayloadType>( static_cast<UInt32>( TMessageTraits<PayloadType>::Type ),
                                       static_cast<UInt16>( TMessageTraits<PayloadType>::Version ) );
}

template <Lumen::Concepts::CMessagePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] inline Lumen::TOptional<PayloadType> Lumen::FMessage::TryGetPayload ( const UInt32 InType,
                                                                                    const UInt16 InVersion ) const noexcept
{
    if ( PayloadSize != sizeof( PayloadType ) || Type != InType || Version != InVersion )
    {
        return {};
    }

    std::array<Byte, sizeof( PayloadType )> RawData{};
    std::memcpy( RawData.data(), PayloadData, sizeof( PayloadType ) );

    return Lumen::TOptional<PayloadType>( std::bit_cast<PayloadType>( RawData ) );
}
