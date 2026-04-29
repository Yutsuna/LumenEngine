/**
 * @file Hash.hpp
 * @brief Declaration of THasher
 */

#pragma once

#include "Concepts/Hash.hpp"
#include "Container/String.hpp"
#include "Hash/Fnv1a.hpp"

#include <tuple>
#include <utility>

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
    template <Concepts::CHashAlgorithm TAlgorithm = FFnv1a64, Concepts::CHashable ValueType>
    [[nodiscard]] FHashValue CombineHash ( FHashValue InSeed, const ValueType &InValue ) noexcept;

    /**
     * @struct THasher
     * @brief Callable Hash Functor; 'operator()(const ValueType &)' -> FHashValue
     * @details All trivially-copyable tyeps via raw-byte FNV-1a64 by default
     */
    template <typename ValueType> struct THasher
    {
    public:

        /**
         * @brief Hashes InValue using the default algorithm for ValueType
         * non-trivially-copyable types.
         * @param InValue The value to hash
         * @return The hash of InValue
         */
        [[nodiscard]] FHashValue operator()( const ValueType InValue ) const noexcept;
    };

    namespace Details
    {

        template <typename TInt> struct TIntHasher
        {
            [[nodiscard]] LumenEngine::FHashValue operator()( const TInt InValue ) const noexcept;
        };

        constexpr FHashValue WangMix64 ( FHashValue InKey ) noexcept;

    } // namespace Details

#define LUMEN_HASH_INT_SPECIALISATION( Type )                                                                                                                            \
    template <> struct THasher<Type> : Details::TIntHasher<Type>                                                                                                         \
    {                                                                                                                                                                    \
        /* */                                                                                                                                                            \
    }

    LUMEN_HASH_INT_SPECIALISATION( Int8 );
    LUMEN_HASH_INT_SPECIALISATION( Int16 );
    LUMEN_HASH_INT_SPECIALISATION( Int32 );
    LUMEN_HASH_INT_SPECIALISATION( Int64 );
    LUMEN_HASH_INT_SPECIALISATION( UInt8 );
    LUMEN_HASH_INT_SPECIALISATION( UInt16 );
    LUMEN_HASH_INT_SPECIALISATION( UInt32 );
    LUMEN_HASH_INT_SPECIALISATION( UInt64 );

#undef LUMEN_HASH_INT_SPECIALISATION

    template <> struct THasher<Float32>
    {
        [[nodiscard]] FHashValue operator()( const Float32 InValue ) const noexcept;
    };

    template <> struct THasher<Float64>
    {
        [[nodiscard]] FHashValue operator()( const Float64 InValue ) const noexcept;
    };

    template <> struct THasher<Bool>
    {
        [[nodiscard]] FHashValue operator()( const Bool InValue ) const noexcept;
    };

    template <> struct THasher<FString>
    {
        using is_transparent = void;

        [[nodiscard]] FHashValue operator()( const FString &InValue ) const noexcept;
        [[nodiscard]] FHashValue operator()( const FStringView InValue ) const noexcept;
    };

    template <typename ObjectType> struct THasher<ObjectType *>
    {
        [[nodiscard]] FHashValue operator()( const ObjectType *InPtr ) const noexcept;
    };

    template <typename First, typename Second> struct THasher<std::pair<First, Second>>
    {
        [[nodiscard]] FHashValue operator()( const std::pair<First, Second> &InPair ) const noexcept;
    };

    template <typename... Args> struct THasher<std::tuple<Args...>>
    {
        [[nodiscard]] FHashValue operator()( const std::tuple<Args...> &InTuple ) const noexcept;
    };

    /**
     * @brief Compute a hash for InValue using algorithm TAlgo.
     * @tparam TAlgo       Hash algorithm policy (default FFnv1a64).
     * @tparam ObjectType  Any CHashable type.
     * @return 64-bit hash value.
     */
    template <Concepts::CHashAlgorithm Algorithm = FFnv1a64, typename ObjectType> [[nodiscard]] FHashValue Compute ( const ObjectType &InValue ) noexcept;

    /**
     * @brief Compute a FNV-1a hash over a raw byte range.
     * @param InData Pointer to raw data.
     * @param InSize Number of bytes.
     * @param InSeed Optional seed value.
     * @return 64-bit hash value.
     */
    [[nodiscard]] FHashValue ComputeRaw ( const void *InData, USize InSize, FHashValue InSeed = FFnv1a64::OffsetBasis ) noexcept;

    /**
     * @brief Hash adapter compatible with std::unordered_map/std::unordered_set.
     */
    template <typename ValueType> struct TStdHashAdapter
    {
        using is_transparent = void;

        [[nodiscard]] FHashValue operator()( const ValueType &InValue ) const noexcept;
    };

    /**
     * @struct TStdHashAdapter<FString>
     * @brief Specialization of TStdHashAdapter for FString | FStringView
     */
    template <> struct TStdHashAdapter<FString>
    {
        using is_transparent = void;

        [[nodiscard]] FHashValue operator()( const FString &InValue ) const noexcept;
        [[nodiscard]] FHashValue operator()( const FStringView InValue ) const noexcept;
    };

} // namespace Hash

} // namespace LumenEngine

#include "Inline/Hash.inl"
