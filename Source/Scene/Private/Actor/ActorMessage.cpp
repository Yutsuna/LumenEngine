/**
 * @file ActorMessage.cpp
 * @brief FMessage implementation for Lumen.
 */

#include <Actor/ActorMessage.hpp>

Lumen::FMessage Lumen::FMessage::Make ( const UInt32 InType, const ActorID InSender ) noexcept
{
    FMessage Msg;
    Msg.Type   = InType;
    Msg.Sender = InSender;
    return Msg;
}
