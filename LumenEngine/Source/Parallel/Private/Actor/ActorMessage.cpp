/**
 * @file ActorMessage.cpp
 * @brief FMessage implementation for LumenEngine.
 */

#include "Actor/ActorMessage.hpp"

LumenEngine::FMessage LumenEngine::FMessage::Make ( UInt32 InType, ActorID InSender, FAny InPayload ) noexcept
{
    FMessage Message;
    Message.Type    = InType;
    Message.Sender  = InSender;
    Message.Payload = std::move( InPayload );
    return Message;
}