/**
 * @file ActorMessage.hpp
 * @brief ActorMessage struct definition for parallel processing in Lumen Engine.
 */

#pragma once

#include "Actor/ActorTypes.hpp"

namespace LumenEngine
{

/**
 * @struct FMessage
 * @brief Represents a message sent between actors in the Lumen Engine.
 * @details Sized to exactly 128 bytes to fit perfectly in 2 CPU cache lines.
 */
struct alignas( 16 ) FMessage
{
    /**
     * Header (16 bytes total)
     * We put Sender first to avoid internal padding.
     */
    ActorID Sender = 0ULL; //<<< 8 bytes
    UInt32 Type    = 0U;   //<<< 4 bytes
    UInt32 Padding = 0U;   //<<< 4 bytes (Explicit padding to reach 16 bytes)

    /**
     * @brief Buffer of inline payload data (112 bytes)
     * 16 (header) + 112 = 128 bytes.
     */
    Byte PayloadData[112] = {};

    /** @brief Create a message with a payload */
    template <typename PayloadType> static FMessage Make ( UInt32 InType, ActorID InSender, const PayloadType &InPayload ) noexcept;

    /** @brief Create a message without a payload */
    static FMessage Make ( UInt32 InType, ActorID InSender ) noexcept;

    /** @brief Read the payload directly from the inline buffer */
    template <typename PayloadType> const PayloadType &GetPayload () const noexcept;

    static constexpr std::size_t MaxPayloadSize = sizeof( PayloadData );
};

static_assert( sizeof( FMessage ) == 128, "FMessage should be exactly 128 bytes for performance reasons." );

} // namespace LumenEngine

#include "Inline/ActorMessage.inl"