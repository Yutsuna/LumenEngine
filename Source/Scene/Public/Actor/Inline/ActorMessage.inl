/// @file ActorMessage.inl
/// @brief Inline implementation for ActorMessage in Lumen Engine

#pragma once

#include <Actor/ActorMessage.hpp>

#include <cstring>

template <Lumen::Concepts::CTriviallyCopyablePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] Lumen::FMessage Lumen::FMessage::Make ( UInt32 InType, ActorID InSender, const PayloadType &InPayload ) noexcept
{
    FMessage Message;
    Message.Type   = InType;
    Message.Sender = InSender;

    std::memcpy( Message.PayloadData, &InPayload, sizeof( PayloadType ) );
    return Message;
}

[[nodiscard]] inline Lumen::FMessage Lumen::FMessage::Make ( UInt32 InType, ActorID InSender ) noexcept
{
    FMessage Message;
    Message.Type   = InType;
    Message.Sender = InSender;

    return Message;
}

template <Lumen::Concepts::CTriviallyCopyablePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] inline const PayloadType &Lumen::FMessage::GetPayload () const noexcept
{
    return *reinterpret_cast<const PayloadType *>( PayloadData );
}
