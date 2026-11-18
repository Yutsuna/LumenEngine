/**
 * @file RHIHash.inl
 * @brief Inline templates for RHI content hashing.
 */

#pragma once

#include <RHI/Types/RHIHash.hpp>
#include <memory>

/**
 * Public
 */

template <typename ValueType>
    requires Lumen::Concepts::CTriviallyCopyable<ValueType>
Lumen::UInt64 Lumen::RHI::RHIHashValue ( const ValueType &InValue ) noexcept
{
    const Byte *Bytes = reinterpret_cast<const Byte *>( std::addressof( InValue ) );
    return RHIHashBytes( TSpan<const Byte>( Bytes, sizeof( ValueType ) ) );
}

template <typename ValueType>
Lumen::USize Lumen::RHI::TRHIValueHasher<ValueType>::operator()( const ValueType &InValue ) const noexcept
{
    return static_cast<USize>( RHIHashValue( InValue ) );
}
