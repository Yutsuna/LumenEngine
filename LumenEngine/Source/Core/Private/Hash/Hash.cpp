/**
 * @file Hash.cpp
 * @brief Implementation of Hash functions and functors in LumenEngine
 */

#include "Hash/Hash.hpp"

LumenEngine::FHashValue LumenEngine::Hash::ComputeRaw ( const void *InData, USize InSize, FHashValue InSeed ) noexcept
{
    FFnv1a64 Algorithm( InSeed );

    Algorithm.Write( InData, InSize );
    return Algorithm.Digest();
}