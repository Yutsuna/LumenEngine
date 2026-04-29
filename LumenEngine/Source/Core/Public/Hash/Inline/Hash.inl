/**
 * @file Hash.inl
 * @brief Inline implementations for Hash.hpp
 */

#pragma once

#include "Hash/Fnv1a.hpp"
#include "Hash/Hash.hpp"
#include <cstring>

namespace LumenEngine
{

template <Concepts::CHashAlgorithm TAlgorithm, Concepts::CHashable ValueType>
[[nodiscard]] FHashValue Hash::CombineHash ( FHashValue InSeed, const ValueType &InValue ) noexcept
{
    const FHashValue ValueHash = Compute<TAlgorithm>( InValue );

    constexpr FHashValue Magic = 0x9E3779B97F4A7C15ULL;
    return InSeed ^ ( ValueHash + Magic + ( InSeed << 6U ) + ( InSeed >> 2U ) );
}

template <typename ValueType> FHashValue Hash::THasher<ValueType>::operator()( const ValueType InValue ) const noexcept
{
    FFnv1a64 Algorithm;

    Algorithm.Write( &InValue, sizeof( ValueType ) );
    return Algorithm.Digest();
}

/**
 * Details
 */

constexpr FHashValue Hash::Details::WangMix64 ( FHashValue InKey ) noexcept
{
    InKey = ( ~InKey ) + ( InKey << 21U );
    InKey = InKey ^ ( InKey >> 24U );
    InKey = ( InKey + ( InKey << 3U ) ) + ( InKey << 8U );
    InKey = InKey ^ ( InKey >> 14U );
    InKey = ( InKey + ( InKey << 2U ) ) + ( InKey << 4U );
    InKey = InKey ^ ( InKey >> 28U );
    InKey = InKey + ( InKey << 31U );
    return InKey;
}

template <typename TInt> FHashValue Hash::Details::TIntHasher<TInt>::operator()( const TInt InValue ) const noexcept
{
    return WangMix64( static_cast<FHashValue>( InValue ) );
}

/**
 * THasher Specializations
 */

[[nodiscard]] inline FHashValue Hash::THasher<Float32>::operator()( const Float32 InValue ) const noexcept
{
    const Float32 Normalized = ( InValue == 0.0F ) ? 0.0F : InValue;
    UInt32 Bits{};
    std::memcpy( &Bits, &Normalized, sizeof( Float32 ) );
    return Details::WangMix64( static_cast<FHashValue>( Bits ) );
}

[[nodiscard]] inline FHashValue Hash::THasher<Float64>::operator()( const Float64 InValue ) const noexcept
{
    const Float64 Normalized = ( InValue == 0.0 ) ? 0.0 : InValue;
    UInt64 Bits{};
    std::memcpy( &Bits, &Normalized, sizeof( Float64 ) );
    return Details::WangMix64( static_cast<FHashValue>( Bits ) );
}

[[nodiscard]] inline FHashValue Hash::THasher<Bool>::operator()( const Bool InValue ) const noexcept
{
    return InValue ? 0X1ULL : 0X0ULL;
}

[[nodiscard]] inline FHashValue Hash::THasher<FString>::operator()( const FString &InValue ) const noexcept
{
    FFnv1a64 Algorithm;
    Algorithm.Write( InValue.data(), InValue.size() );
    return Algorithm.Digest();
}

[[nodiscard]] inline FHashValue Hash::THasher<FString>::operator()( const FStringView InValue ) const noexcept
{
    FFnv1a64 Algorithm;
    Algorithm.Write( InValue.data(), InValue.size() );
    return Algorithm.Digest();
}

/**
 * Partial Specializations
 */

template <typename ObjectType> [[nodiscard]] FHashValue Hash::THasher<ObjectType *>::operator()( const ObjectType *InPtr ) const noexcept
{
    return Details::WangMix64( reinterpret_cast<FHashValue>( InPtr ) );
}

template <typename First, typename Second>
[[nodiscard]] FHashValue Hash::THasher<std::pair<First, Second>>::operator()( const std::pair<First, Second> &InPair ) const noexcept
{
    const FHashValue H1 = THasher<First>{}( InPair.first );

    return CombineHash( H1, InPair.second );
}

template <typename... Args> [[nodiscard]] FHashValue Hash::THasher<std::tuple<Args...>>::operator()( const std::tuple<Args...> &InTuple ) const noexcept
{
    FHashValue Seed = FFnv1a64::OffsetBasis;

    std::apply( [&Seed] ( const Args &...InArgs ) { ( ( Seed = CombineHash( Seed, InArgs ) ), ... ); }, InTuple );
    return Seed;
}

template <Concepts::CHashAlgorithm Algorithm, typename ObjectType> [[nodiscard]] FHashValue Hash::Compute ( const ObjectType &InValue ) noexcept
{
    Algorithm Algo;
    if constexpr ( Concepts::CHasHasherSpecialisation<ObjectType> )
    {
        return THasher<ObjectType>{}( InValue );
    }
    else
    {
        Algo.Write( &InValue, sizeof( ObjectType ) );
        return Algo.Digest();
    }
}

template <typename ValueType> FHashValue Hash::TStdHashAdapter<ValueType>::operator()( const ValueType &InValue ) const noexcept
{
    return Compute( InValue );
}

[[nodiscard]] inline FHashValue Hash::TStdHashAdapter<FString>::operator()( const FString &InValue ) const noexcept
{
    return Compute( InValue );
}

[[nodiscard]] inline FHashValue Hash::TStdHashAdapter<FString>::operator()( const FStringView InValue ) const noexcept
{
    return Compute( InValue );
}

} // namespace LumenEngine
