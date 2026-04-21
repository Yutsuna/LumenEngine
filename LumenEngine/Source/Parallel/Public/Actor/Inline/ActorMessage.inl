/**
 * @file ActorMessage.inl
 * @brief Inline implementation for ActorMessage in Lumen Engine.
 */

#pragma once

#include "Actor/ActorMessage.hpp"

#include <cstring>

template <LumenEngine::Concepts::CTriviallyCopyablePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] LumenEngine::FMessage LumenEngine::FMessage::Make ( UInt32 InType, ActorID InSender, const PayloadType &InPayload ) noexcept
{
    FMessage Msg;
    Msg.Type   = InType;
    Msg.Sender = InSender;

    std::memcpy( Msg.PayloadData, &InPayload, sizeof( PayloadType ) );

    return Msg;
}

[[nodiscard]] inline LumenEngine::FMessage LumenEngine::FMessage::Make ( UInt32 InType, ActorID InSender ) noexcept
{
    FMessage Msg;
    Msg.Type   = InType;
    Msg.Sender = InSender;
    return Msg;
}

template <LumenEngine::Concepts::CTriviallyCopyablePayload PayloadType>
    requires( sizeof( PayloadType ) <= 112 )
[[nodiscard]] const PayloadType &LumenEngine::FMessage::GetPayload () const noexcept
{
    return *reinterpret_cast<const PayloadType *>( PayloadData );
}
