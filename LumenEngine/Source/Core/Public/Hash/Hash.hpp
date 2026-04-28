/**
 * @file Hash.hpp
 * @brief Concepts for Hash algorithms in LumenEngine
 */

#pragma once

#include "Concepts/Hash.hpp"
#include "Hash/Fnv1a.hpp"

namespace LumenEngine
{

namespace Hash
{

    /**
     * @brief Combine InHas with the hash of InValue
     * @details Uses the FFnv1a64 algorithm by default
     * @tparam TAlgorithm Hash algorithm to use for hashing InValue
     * @tparam ValueType Any CHashable type
     */
    template <Concepts::CHashAlgorithm = FFnv1a64, Concepts::CHashable ValueType>
    [[nodiscard]] FHashValue CombineHash ( FHashValue InSeed, const ValueType &InValue ) noexcept
    {
        FFnv1a64 Algorithm{ InSeed };

        Algorithm.Write( &InValue, sizeof( ValueType ) );
        return Algorithm.Digest();
    }

    /**
     * @struct THasher
     * @brief Callable Hash Functor; 'operator()(const ValueType &)' -> FHashValue
     * @details All trivially-copyable tyeps via raw-byte FNV-1a64 by default
     */
    template <typename ValueType> struct THasher
    {
        static_assert( Concepts::CTriviallyCopyable<ValueType>,
                       "THasher requires a trivially-copyable type by default. "
                       "Provide a specialization or an ADL HashValue() function." );

    public:

        /**
         * @brief Hashes InValue using the default algorithm for ValueType
         * non-trivially-copyable types.
         * @param InValue The value to hash
         * @return The hash of InValue
         */
        [[nodiscard]] FHashValue operator()( const ValueType &InValue ) const noexcept;
    };

} // namespace Hash

} // namespace LumenEngine

#include "Inline/Hash.inl"
