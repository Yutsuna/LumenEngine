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

constexpr void LumenEngine::Hash::FFnv1a64::Write ( const Byte *DataPtr, const USize InSize ) noexcept
{
    for ( USize Index = 0; Index < InSize; ++Index )
    {
        HashState ^= static_cast<FHashValue>( DataPtr[Index] );
        HashState *= Prime;
    }
}

constexpr void LumenEngine::Hash::FFnv1a64::Write ( const AnsiChar *InDataPtr, const USize InSize ) noexcept
{
    for ( USize Index = 0; Index < InSize; ++Index )
    {
        HashState ^= static_cast<FHashValue>( static_cast<unsigned char>( InDataPtr[Index] ) );
        HashState *= Prime;
    }
}

inline void LumenEngine::Hash::FFnv1a64::Write ( const void *InDataPtr, const USize InSize ) noexcept
{
    Write( static_cast<const Byte *>( InDataPtr ), InSize );
}

constexpr LumenEngine::FHashValue LumenEngine::Hash::FFnv1a64::Digest () const noexcept
{
    return HashState;
}

constexpr void LumenEngine::Hash::FFnv1a64::Reset ( const FHashValue InSeed ) noexcept
{
    HashState = InSeed;
}
