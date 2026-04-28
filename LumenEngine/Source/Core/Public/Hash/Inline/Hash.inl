/**
 * @file Hash.inl
 * @brief Inline implementations for Hash.hpp
 */

#pragma once

#include "Hash/Hash.hpp"

template <typename ValueType> LumenEngine::FHashValue LumenEngine::Hash::THasher<ValueType>::operator()( const ValueType &InValue ) const noexcept
{
    FFnv1a64 Algorithm;

    Algorithm.Write( &InValue, sizeof( ValueType ) );
    return Algorithm.Digest();
}
