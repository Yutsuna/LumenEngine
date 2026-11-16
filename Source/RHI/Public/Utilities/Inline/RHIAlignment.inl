/**
 * @file RHIAlignment.inl
 * @brief Inline alignment and range helpers.
 */
#pragma once

#include <Utilities/RHIAlignment.hpp>
#include <Utilities/RHICheckedMath.hpp>

template <typename ValueType>
    requires std::is_unsigned_v<ValueType>
constexpr Lumen::Bool Lumen::RHIIsPowerOfTwo ( const ValueType InValue ) noexcept
{
    return InValue != 0U and ( InValue & ( InValue - 1U ) ) == 0U;
}

template <typename ValueType>
    requires std::is_unsigned_v<ValueType>
constexpr Lumen::TOptional<ValueType> Lumen::RHIAlignUp ( const ValueType InValue, const ValueType InAlignment ) noexcept
{
    if ( not RHIIsPowerOfTwo( InAlignment ) )
    {
        return {};
    }
    const TOptional<ValueType> WithPadding = RHICheckedAdd( InValue, static_cast<ValueType>( InAlignment - 1U ) );
    if ( not WithPadding )
    {
        return {};
    }
    return TOptional<ValueType>( static_cast<ValueType>( *WithPadding & ~( InAlignment - 1U ) ) );
}

template <typename ValueType>
    requires std::is_unsigned_v<ValueType>
constexpr Lumen::TOptional<ValueType> Lumen::RHIAlignDown ( const ValueType InValue, const ValueType InAlignment ) noexcept
{
    if ( not RHIIsPowerOfTwo( InAlignment ) )
    {
        return {};
    }
    return TOptional<ValueType>( static_cast<ValueType>( InValue & ~( InAlignment - 1U ) ) );
}

constexpr Lumen::Bool Lumen::RHIRangesOverlap ( const UInt64 InFirstOffset,
                                                const UInt64 InFirstSize,
                                                const UInt64 InSecondOffset,
                                                const UInt64 InSecondSize ) noexcept
{
    if ( InFirstSize == 0ULL or InSecondSize == 0ULL )
    {
        return false;
    }
    const TOptional<UInt64> FirstEnd  = RHICheckedAdd( InFirstOffset, InFirstSize );
    const TOptional<UInt64> SecondEnd = RHICheckedAdd( InSecondOffset, InSecondSize );
    return FirstEnd and SecondEnd and InFirstOffset < *SecondEnd and InSecondOffset < *FirstEnd;
}
