/**
 * @file ActorMessage.hpp
 * @brief ActorMessage struct definition.
 */

#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorTypes.hpp>

#include <cstring>
#include <memory>

namespace Lumen
{

/**
 * @struct FMessage
 * @brief Represents a message sent between actors in the Lumen Engine.
 * @details Sized to exactly 128 bytes and aligned to 64 bytes to fit perfectly in 2 CPU cache lines.
 */
struct alignas( 64 ) FMessage
{
    static constexpr USize MaxPayloadSize   = 112;
    static constexpr USize PayloadAlignment = 16;

    ActorID Sender                                               = 0ULL;
    UInt32 Type                                                  = 0U;
    UInt16 PayloadSize                                           = 0U; //<< sizeof(PayloadType) when Make<T> was used, 0 otherwise
    UInt16 Flags                                                 = 0U;
    alignas( PayloadAlignment ) Byte PayloadData[MaxPayloadSize] = {};

    /**
     * @brief Creates a message with the given type, sender, and payload.
     * @param InType The type of the message.
     * @param InSender The sender of the message.
     * @param InPayload The payload of the message.
     * @return A message with the given type, sender, and payload.
     */
    template <Concepts::CTriviallyCopyablePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= MaxPayloadSize and alignof( TPayloadType ) <= PayloadAlignment )
    [[nodiscard]] static FMessage Make ( UInt32 InType, ActorID InSender, const TPayloadType &InPayload ) noexcept
    {
        FMessage Msg;
        Msg.Type        = InType;
        Msg.Sender      = InSender;
        Msg.PayloadSize = static_cast<UInt16>( sizeof( TPayloadType ) );
        std::memcpy( Msg.PayloadData, &InPayload, sizeof( TPayloadType ) );
        return Msg;
    }

    /**
     * @brief Creates a message with the given type and sender.
     * @param InType The type of the message.
     * @param InSender The sender of the message.
     * @return A message with the given type and sender.
     */
    [[nodiscard]] static FMessage Make ( UInt32 InType, ActorID InSender ) noexcept
    {
        FMessage Msg;
        Msg.Type   = InType;
        Msg.Sender = InSender;
        return Msg;
    }

    /**
     * @brief Gets the payload of the message.
     * @return The payload of the message.
     */
    template <Concepts::CTriviallyCopyablePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= MaxPayloadSize and alignof( TPayloadType ) <= PayloadAlignment )
    [[nodiscard]] const TPayloadType &GetPayload () const noexcept
    {
#if defined( __cpp_lib_start_lifetime_as ) && __cpp_lib_start_lifetime_as >= 202207L
        return *std::start_lifetime_as<const TPayloadType>( PayloadData );
#else
        thread_local TPayloadType Storage;
        std::memcpy( &Storage, PayloadData, sizeof( TPayloadType ) );
        return Storage;
#endif
    }
};

static_assert( sizeof( FMessage ) == 128, "FMessage should be exactly 128 bytes." );
static_assert( alignof( FMessage ) == 64, "FMessage should be aligned to 64 bytes." );
static_assert( offsetof( FMessage, PayloadData ) == 16, "PayloadData should be at offset 16." );

} // namespace Lumen

/// #include "Inline/ActorMessage.inl"
