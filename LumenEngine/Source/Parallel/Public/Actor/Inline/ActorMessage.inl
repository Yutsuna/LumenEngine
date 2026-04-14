/**
 * @file ActorMessage.inl
 * @brief Inline implementation for ActorMessage in Lumen Engine.
 */

#pragma once

#include "Actor/ActorMessage.hpp"

#include <cstring>
#include <type_traits>

template <typename PayloadType> LumenEngine::FMessage LumenEngine::FMessage::Make ( UInt32 InType, ActorID InSender, const PayloadType &InPayload ) noexcept
{
    static_assert( std::is_trivially_copyable_v<std::remove_cvref_t<PayloadType>>,
                   "FMessage::Make: Payload must be trivially copyable (No virtual functions, no complex containers)" );

    static_assert( sizeof( PayloadType ) <= MaxPayloadSize, "FMessage::Make: Payload size exceeds maximum allowed size (112 bytes)" );

    FMessage Msg;
    Msg.Type   = InType;
    Msg.Sender = InSender;

    std::memcpy( Msg.PayloadData, &InPayload, sizeof( PayloadType ) );

    return Msg;
}

inline LumenEngine::FMessage LumenEngine::FMessage::Make ( UInt32 InType, ActorID InSender ) noexcept
{
    FMessage Msg;
    Msg.Type   = InType;
    Msg.Sender = InSender;
    return Msg;
}

template <typename PayloadType> const PayloadType &LumenEngine::FMessage::GetPayload () const noexcept
{
    static_assert( sizeof( PayloadType ) <= FMessage::MaxPayloadSize, "PayloadType requested is larger than the message buffer" );

    return *reinterpret_cast<const PayloadType *>( PayloadData );
}