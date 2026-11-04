///
/// @file ActorMessage.hpp
/// @brief ActorMessage struct definition for parallel processing in Lumen Engine.
///

#pragma once

#include <Actor/ActorTypes.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

///
/// @struct FMessage
/// @brief Represents a message sent between actors in the Lumen Engine.
/// @details Sized to exactly 128 bytes and aligned to 64 bytes to fit perfectly in 2 CPU cache lines.
/// Type, Version, and PayloadSize validate a payload schema, not C++ type identity. Callers providing manual identifiers
/// must maintain the same unique {Type, Version} schema mapping required from TMessageTraits specializations.
///
struct alignas( 64 ) FMessage
{
    ActorID Sender     = {};
    UInt32 Type        = 0U;
    UInt16 PayloadSize = 0U;
    UInt16 Version     = 0U;

    Byte PayloadData[112] = {};

    /// @brief Create a manually identified message with version zero.
    template <Concepts::CMessagePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= 112 )
    [[nodiscard]] static FMessage Make ( UInt32 InType, ActorID InSender, const TPayloadType &InPayload ) noexcept;

    /// @brief Create a manually identified and versioned message.
    template <Concepts::CMessagePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= 112 )
    [[nodiscard]] static FMessage
    Make ( UInt32 InType, UInt16 InVersion, ActorID InSender, const TPayloadType &InPayload ) noexcept;

    /// @brief Create a message using the identifier and version from TMessageTraits.
    template <Concepts::CIdentifiedMessagePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= 112 )
    [[nodiscard]] static FMessage Make ( ActorID InSender, const TPayloadType &InPayload ) noexcept;

    /// @brief Create a message without a payload.
    [[nodiscard]] static FMessage Make ( UInt32 InType, ActorID InSender ) noexcept;

    /// @brief Copy a trait-identified payload after checking its type, version, and size.
    /// @note A mismatch is a programmer error and terminates the process. Use TryGetPayload to handle it.
    template <Concepts::CIdentifiedMessagePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= 112 )
    [[nodiscard]] TPayloadType GetPayload () const noexcept;

    /// @brief Copy a manually identified payload after checking its type, version, and size.
    template <Concepts::CMessagePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= 112 )
    [[nodiscard]] TPayloadType GetPayload ( UInt32 InType, UInt16 InVersion = 0U ) const noexcept;

    /// @brief Copy a payload if its recorded metadata matches TMessageTraits<TPayloadType>.
    template <Concepts::CIdentifiedMessagePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= 112 )
    [[nodiscard]] TOptional<TPayloadType> TryGetPayload () const noexcept;

    /// @brief Copy a payload if its recorded metadata matches all the supplied values.
    template <Concepts::CMessagePayload TPayloadType>
        requires( sizeof( TPayloadType ) <= 112 )
    [[nodiscard]] TOptional<TPayloadType> TryGetPayload ( UInt32 InType, UInt16 InVersion = 0U ) const noexcept;

    static constexpr USize MaxPayloadSize = sizeof( PayloadData );
};

static_assert( sizeof( FMessage ) == 128, "FMessage should be exactly 128 bytes for performance reasons." );

} // namespace Lumen

#include "Inline/ActorMessage.inl"
