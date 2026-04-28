/**
 * @file Fnv1a.inl
 * @brief Inline definitions for FNV-1a hash implementation.
 */

#pragma once

#include "Hash/Fnv1a.hpp"

constexpr LumenEngine::Hash::FFnv1a64::FFnv1a64 () noexcept : HashState( OffsetBasis )
{
    /* Ctor */
}

constexpr LumenEngine::Hash::FFnv1a64::FFnv1a64 ( const FHashValue InSeed ) noexcept : HashState( InSeed )
{
    /* Ctor */
}

constexpr void LumenEngine::Hash::FFnv1a64::Write ( const void *InData, const USize InSize ) noexcept
{
    const UInt8 *Bytes = static_cast<const UInt8 *>( InData );

    for ( USize Index = 0; Index < InSize; ++Index )
    {
        HashState ^= Bytes[Index];
        HashState *= Prime;
    }
}

constexpr LumenEngine::FHashValue LumenEngine::Hash::FFnv1a64::Digest () const noexcept
{
    return HashState;
}

constexpr void LumenEngine::Hash::FFnv1a64::Reset ( const FHashValue InSeed ) noexcept
{
    HashState = InSeed;
}
