/**
 * @file ActorMessage.hpp
 * @brief ActorMessage struct definition.
 */

#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorTypes.hpp>

namespace Lumen
{

/** INFO: Payload limits live at namespace scope so the requires-clauses below can also reference them from the out-of-line
 * definitions in Inline/ActorMessage.inl (GCC cannot match a requires-clause that references class-scope constants across an
 * out-of-line definition). */
inline constexpr SCENE_EXPORT USize FMessageMaxPayloadSize   = 112;
inline constexpr SCENE_EXPORT USize FMessagePayloadAlignment = 16;

/**
 * @struct FMessage
 * @brief Represents a message sent between actors in the Lumen Engine.
 * @details Sized to exactly 128 bytes and aligned to 64 bytes to fit perfectly in 2 CPU cache lines.
 */
struct alignas( 64 ) SCENE_EXPORT FMessage
{
    static constexpr USize MaxPayloadSize   = FMessageMaxPayloadSize;
    static constexpr USize PayloadAlignment = FMessagePayloadAlignment;

    ActorID Sender     = 0ULL;
    UInt32 Type        = 0U;
    UInt16 PayloadSize = 0U; //<< sizeof(PayloadType) when Make<T> was used, 0 otherwise
    UInt16 Flags       = 0U;

    /** INFO: mutable because std::start_lifetime_as<const T> requires a const pointer as an asm output */
    alignas( PayloadAlignment ) mutable Byte PayloadData[MaxPayloadSize] = {};

    /**
     * @brief Creates a message with the given type, sender, and payload.
     * @param InType The type of the message.
     * @param InSender The sender of the message.
     * @param InPayload The payload of the message.
     * @return A message with the given type, sender, and payload.
     */
    template <Concepts::CTriviallyCopyablePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= Lumen::FMessageMaxPayloadSize and
                  alignof( TPayloadType ) <= Lumen::FMessagePayloadAlignment )
    [[nodiscard]] static FMessage Make ( UInt32 InType, ActorID InSender, const TPayloadType &InPayload ) noexcept;

    /**
     * @brief Creates a message with the given type and sender.
     * @param InType The type of the message.
     * @param InSender The sender of the message.
     * @return A message with the given type and sender.
     */
    [[nodiscard]] static FMessage Make ( UInt32 InType, ActorID InSender ) noexcept;

    /**
     * @brief Gets the payload of the message.
     * @return The payload of the message.
     */
    template <Concepts::CTriviallyCopyablePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= Lumen::FMessageMaxPayloadSize and
                  alignof( TPayloadType ) <= Lumen::FMessagePayloadAlignment )
    [[nodiscard]] TPayloadType GetPayload () const noexcept;
};

static_assert( sizeof( FMessage ) == 128, "FMessage should be exactly 128 bytes." );
static_assert( alignof( FMessage ) == 64, "FMessage should be aligned to 64 bytes." );
static_assert( offsetof( FMessage, PayloadData ) == 16, "PayloadData should be at offset 16." );

} // namespace Lumen

#include "Inline/ActorMessage.inl"
