/**
 * @file ActorMessage.hpp
 * @brief ActorMessage struct definition for parallel processing in Lumen Engine.
 */

#pragma once

#include "Actor/ActorTypes.hpp"

#include "Container/Any.hpp"

namespace LumenEngine
{

/**
 * @struct FMessage
 * @brief Represents a message sent between actors in the Lumen Engine.
 */
struct FMessage
{
    UInt32 Type = 0U;
    FAny Payload;
    ActorID Sender = 0ULL;
};

} // namespace LumenEngine