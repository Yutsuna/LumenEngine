/**
 * @file Hash.hpp
 * @brief Concepts for Hash algorithms in LumenEngine
 */

#pragma once

#include "Concepts/DefaultInitializable.hpp"
#include "Concepts/EqualityComparable.hpp"
#include "Concepts/TriviallyCopyable.hpp"
#include "CoreTypes.hpp"

#include "Concepts/ConvertibleTo.hpp"
#include "Concepts/SameAs.hpp"

#include <concepts>

namespace LumenEngine
{

/**
 * @typedef FHashValue
 * @brief Canonical 64-bit hash value type
 */
using FHashValue = UInt64;

template <typename ValueType> struct THasher;

namespace Concepts
{

    /**
     * @concept CHashAlgorithm
     * @brief Constrains a stateful incremental hash algorithm
     *
     * @tparam TAlgorithm The Algorithm type
     *
     * @code
     *   TAlgorithm Algorithm {};
     *   Algorithm.Write(Pointer, Size);
     *   FHashValue Hash = Algorithm.Digest();
     * @endcode
     */
    template <typename TAlgorithm>
    concept CHashAlgorithm = CDefaultInitializable<TAlgorithm> and requires( TAlgorithm &Algorithm, const void *Pointer, USize Size ) {
        { Algorithm.Write( Pointer, Size ) } -> CSameAs<void>;
        { Algorithm.Digest() } -> CConvertibleTo<FHashValue>;
    };

    /**
     * @concept CHasHasherSpecialisation
     * @brief Constrains types for which a specialization of THasher<ValueType> exists and is convertible to FHashValue
     */
    template <typename ValueType>
    concept CHasHasherSpecialisation = requires( const ValueType &Value ) {
        { THasher<ValueType>{} } -> CConvertibleTo<FHashValue>;
    };

    /**
     * @concept Detects ADL free function
     * @brief Constrains types for which a free function HashValue( const ValueType & ) is found via ADL and returns a type convertible to FHashValue
     */
    template <typename ValueType>
    concept CHasADLHash = requires( const ValueType &Value ) {
        { HashValue( Value ) } -> CConvertibleTo<FHashValue>;
    };

    /**
     * @concept CHashable
     * @brief A type that can produce a deterministic 64-bit hash
     */
    template <typename ValueType>
    concept CHashable = CTriviallyCopyable<ValueType> or CHasHasherSpecialisation<ValueType> or CHasADLHash<ValueType>;

    /**
     * @concept CCacheKey
     * @brief A type suitable as a cache key: Hashable + EqualityComparable
     */
    template <typename ValueType>
    concept CCacheKey = CHashable<ValueType> and CEqualityComparable<ValueType>;

} // namespace Concepts

} // namespace LumenEngine
