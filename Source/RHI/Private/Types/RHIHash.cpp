/**
 * @file RHIHash.cpp
 * @brief Implements stable content hashing.
 */

#include <Types/RHIHash.hpp>

Lumen::UInt64 Lumen::RHIHashBytes ( const TSpan<const Byte> InBytes ) noexcept
{
    UInt64 Hash = 0xCBF29CE484222325ULL;
    for ( const Byte Value : InBytes )
    {
        Hash ^= static_cast<UInt64>( Value );
        Hash *= 0x100000001B3ULL;
    }
    return Hash;
}

Lumen::UInt64 Lumen::RHIHashCombine ( const UInt64 InSeed, const UInt64 InValue ) noexcept
{
    return InSeed ^ ( InValue + 0x9E3779B97F4A7C15ULL + ( InSeed << 6U ) + ( InSeed >> 2U ) );
}
