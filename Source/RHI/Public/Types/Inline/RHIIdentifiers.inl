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
    UInt64 Current = NextValue.load( std::memory_order_relaxed );

    /** NOTE: Zero is the permanent exhausted latch; no further identifier may be issued. */
    while ( Current != 0ULL )
    {
        const UInt64 Issued    = Current;
        const UInt64 Following = Current == std::numeric_limits<UInt64>::max() ? 0ULL : Current + 1ULL;

        /** NOTE: compare_exchange_weak refreshes Current with the observed value on failure. */
        if ( NextValue.compare_exchange_weak( Current, Following, std::memory_order_relaxed, std::memory_order_relaxed ) )
        {
            return TRHIId<TagType>{ Issued };
        }
    }

    return TRHIId<TagType>{ 0ULL };
}
