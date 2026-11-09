/**
 * @file ActorMessage.inl
 * @brief Actor message inline implementation
 */

#pragma once

#include <Actor/ActorMessage.hpp>

#include <cstring>
#include <memory>

/**
 * FMessage
 */

template <Lumen::Concepts::CTriviallyCopyablePayload TPayloadType>
    requires( sizeof( TPayloadType ) <= Lumen::FMessage::MaxPayloadSize and
              alignof( TPayloadType ) <= Lumen::FMessage::PayloadAlignment )
Lumen::FMessage Lumen::FMessage::Make ( const UInt32 InType, const ActorID InSender, const TPayloadType &InPayload ) noexcept
{
    FMessage Msg;
    Msg.Type        = InType;
    Msg.Sender      = InSender;
    Msg.PayloadSize = static_cast<UInt16>( sizeof( TPayloadType ) );
    std::memcpy( Msg.PayloadData, &InPayload, sizeof( TPayloadType ) );
    return Msg;
}

Lumen::FMessage Lumen::FMessage::Make ( const UInt32 InType, const ActorID InSender ) noexcept
{
    FMessage Msg;
    Msg.Type   = InType;
    Msg.Sender = InSender;
    return Msg;
}

template <Lumen::Concepts::CTriviallyCopyablePayload TPayloadType>
    requires( sizeof( TPayloadType ) <= Lumen::FMessage::MaxPayloadSize and
              alignof( TPayloadType ) <= Lumen::FMessage::PayloadAlignment )
[[nodiscard]] TPayloadType Lumen::FMessage::GetPayload () const noexcept
{
#if defined( __cpp_lib_start_lifetime_as ) && __cpp_lib_start_lifetime_as >= 202207L
    return *std::start_lifetime_as<TPayloadType>( PayloadData );
#else
    TPayloadType Storage;
    std::memcpy( &Storage, PayloadData, sizeof( TPayloadType ) );
    return Storage;
#endif
}
