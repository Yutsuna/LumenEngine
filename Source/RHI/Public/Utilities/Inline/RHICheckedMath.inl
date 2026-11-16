/**
 * @file RHICheckedMath.inl
 * @brief Inline overflow-checked arithmetic.
 */
#pragma once

#include <Utilities/RHICheckedMath.hpp>

template <typename ValueType>
    requires std::is_unsigned_v<ValueType>
constexpr Lumen::TOptional<ValueType> Lumen::RHICheckedAdd ( const ValueType InLeft, const ValueType InRight ) noexcept
{
    if ( InRight > std::numeric_limits<ValueType>::max() - InLeft )
    {
        return {};
    }
    return TOptional<ValueType>( static_cast<ValueType>( InLeft + InRight ) );
}

template <typename ValueType>
    requires std::is_unsigned_v<ValueType>
constexpr Lumen::TOptional<ValueType> Lumen::RHICheckedMultiply ( const ValueType InLeft, const ValueType InRight ) noexcept
{
    if ( InLeft != 0U and InRight > std::numeric_limits<ValueType>::max() / InLeft )
    {
        return {};
    }
    return TOptional<ValueType>( static_cast<ValueType>( InLeft * InRight ) );
}

template <typename ValueType>
    requires std::is_unsigned_v<ValueType>
constexpr Lumen::TOptional<ValueType> Lumen::RHICheckedDivideCeil ( const ValueType InDividend,
                                                                    const ValueType InDivisor ) noexcept
{
    if ( InDivisor == 0U )
    {
        return {};
    }
    const ValueType Quotient  = static_cast<ValueType>( InDividend / InDivisor );
    const ValueType Remainder = static_cast<ValueType>( InDividend % InDivisor );
    return RHICheckedAdd( Quotient, static_cast<ValueType>( Remainder != 0U ? 1U : 0U ) );
}

template <typename DestinationType, typename SourceType>
    requires std::is_integral_v<DestinationType> and std::is_integral_v<SourceType>
constexpr Lumen::TOptional<DestinationType> Lumen::RHICheckedCast ( const SourceType InValue ) noexcept
{
    if constexpr ( std::is_signed_v<SourceType> == std::is_signed_v<DestinationType> )
    {
        if ( InValue < static_cast<SourceType>( std::numeric_limits<DestinationType>::min() ) or
             InValue > static_cast<SourceType>( std::numeric_limits<DestinationType>::max() ) )
        {
            return {};
        }
    }
    else if constexpr ( std::is_signed_v<SourceType> )
    {
        if ( InValue < 0 )
        {
            return {};
        }
        using UnsignedSource = std::make_unsigned_t<SourceType>;
        if ( static_cast<UnsignedSource>( InValue ) > std::numeric_limits<DestinationType>::max() )
        {
            return {};
        }
    }
    else
    {
        using UnsignedDestination = std::make_unsigned_t<DestinationType>;
        if ( InValue > static_cast<UnsignedDestination>( std::numeric_limits<DestinationType>::max() ) )
        {
            return {};
        }
    }
    return TOptional<DestinationType>( static_cast<DestinationType>( InValue ) );
}
