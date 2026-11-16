/**
 * @file RHIIdentifiers.inl
 * @brief Inline implementation of typed RHI identifiers.
 */
#pragma once

#include <Types/RHIIdentifiers.hpp>
#include <limits>

template <typename TagType> constexpr Lumen::Bool Lumen::TRHIId<TagType>::IsValid () const noexcept
{
    return Value != 0ULL;
}

template <typename TagType> constexpr Lumen::UInt64 Lumen::TRHIId<TagType>::GetValue () const noexcept
{
    return Value;
}

template <typename TagType> Lumen::TRHIId<TagType> Lumen::FRHIIdentifierGenerator::Next () noexcept
{
    const UInt64 Current = NextValue.fetch_add( 1ULL, std::memory_order_relaxed );
    return TRHIId<TagType>{ Current == std::numeric_limits<UInt64>::max() ? 0ULL : Current };
}
