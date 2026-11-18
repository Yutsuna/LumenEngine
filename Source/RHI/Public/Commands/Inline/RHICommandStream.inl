/**
 * @file RHICommandStream.inl
 * @brief Implements typed command stream operations.
 */

#pragma once

#include <Commands/RHICommandStream.hpp>
#include <Concepts/TriviallyCopyable.hpp>
#include <cstring>

/**
 * Public
 */

template <typename PayloadType>
    requires Lumen::Concepts::CTriviallyCopyable<PayloadType>

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHICommandStream::Write ( const UInt32 InOpcode,
                                                                           const PayloadType &InPayload ) noexcept
{
    const Byte *Bytes = reinterpret_cast<const Byte *>( std::addressof( InPayload ) );
    return WritePayload( InOpcode, TSpan<const Byte>( Bytes, sizeof( PayloadType ) ) );
}

template <typename PayloadType>
    requires Lumen::Concepts::CTriviallyCopyable<PayloadType>
Lumen::TExpected<PayloadType, Lumen::FRHIError>

Lumen::FRHICommandStream::ReadPayload ( const USize InIndex, const UInt32 InExpectedOpcode ) const noexcept
{
    return Read( InIndex ).AndThen(
        [InExpectedOpcode] ( const FRHICommandView &InView ) noexcept -> TExpected<PayloadType, FRHIError>
    {
        if ( InView.Opcode != InExpectedOpcode or InView.Payload.size() != sizeof( PayloadType ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHICorrupted( ERHIErrorCondition::Command, "Command payload type or opcode mismatch" ) );
        }

        PayloadType Payload;
        std::memcpy( std::addressof( Payload ), InView.Payload.data(), sizeof( PayloadType ) );
        return Payload;
    } );
}

template <typename ObjectType>

void Lumen::FRHICommandStream::Retain ( const TSharedRef<ObjectType> &InObject ) noexcept
{
    RetainedObjects.push_back( StaticCastSharedRef<IRHIObject>( InObject ) );
}
