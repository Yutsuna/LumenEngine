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
    const auto *Bytes = reinterpret_cast<const Byte *>( std::addressof( InPayload ) );
    return WritePayload( InOpcode, TSpan<const Byte>( Bytes, sizeof( PayloadType ) ) );
}

template <typename PayloadType>
    requires Lumen::Concepts::CTriviallyCopyable<PayloadType>
Lumen::TExpected<PayloadType, Lumen::FRHIError>

Lumen::FRHICommandStream::ReadPayload ( const USize InIndex, const UInt32 InExpectedOpcode ) const noexcept
{
    TExpected<FRHICommandView, FRHIError> View = Read( InIndex );
    if ( not View )
    {
        return View.Error();
    }
    if ( View->Opcode != InExpectedOpcode or View->Payload.size() != sizeof( PayloadType ) )
    {
        return MakeRHICorrupted( ERHIErrorCondition::Command, "Command payload type or opcode mismatch" );
    }

    PayloadType Payload;
    std::memcpy( std::addressof( Payload ), View->Payload.data(), sizeof( PayloadType ) );
    return Payload;
}

template <typename ObjectType>

void Lumen::FRHICommandStream::Retain ( const TSharedRef<ObjectType> &InObject ) noexcept
{
    RetainedObjects.push_back( StaticCastSharedRef<IRHIObject>( InObject ) );
}
