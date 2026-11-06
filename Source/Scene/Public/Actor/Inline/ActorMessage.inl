/**
 * @file ActorMessage.inl
 * @brief Inline implementation for ActorMessage in Lumen Engine.
 */

#pragma once

#include <Actor/ActorMessage.hpp>

#include <cstring>

template <Lumen::Concepts::CTriviallyCopyablePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] Lumen::FMessage Lumen::FMessage::Make ( UInt32 InType, ActorID InSender, const PayloadType &InPayload ) noexcept
{
    FMessage Msg;
    Msg.Type   = InType;
    Msg.Sender = InSender;

    std::memcpy( Msg.PayloadData, &InPayload, sizeof( PayloadType ) );

    return Msg;
}

[[nodiscard]] inline Lumen::FMessage Lumen::FMessage::Make ( UInt32 InType, ActorID InSender ) noexcept
{
    FMessage Msg;
    Msg.Type   = InType;
    Msg.Sender = InSender;
    return Msg;
}

template <Lumen::Concepts::CTriviallyCopyablePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] const PayloadType &Lumen::FMessage::GetPayload () const noexcept
{
    return *reinterpret_cast<const PayloadType *>( PayloadData );
}
