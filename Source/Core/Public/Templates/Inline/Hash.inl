/**
 * @file Hash.inl
 * @brief Inline hash functions implementations.
 */

#pragma once

#include <String/StringView.hpp>
#include <Templates/Hash.hpp>

template <class TObjectType> consteval Lumen::UInt64 Lumen::FHash::TypeHash () noexcept
{

    /**
     * INFO: Use pretty function name as the basis for the hash.
     * because it is stable across TUs, modules, and runs for a given compiler.
     */
    constexpr FStringView PrettyName = __PRETTY_FUNCTION__;
    UInt64 ULHash                    = 0XCBF29CE484222325ULL;

    /**
     * INFO: FNV-1a hash over the pretty-printed type signature.
     */
    for ( const AnsiChar Char : PrettyName )
    {
        ULHash ^= static_cast<UInt64>( Char );
        ULHash *= 0X100000001B3ULL;
    }
    return ULHash;
}
