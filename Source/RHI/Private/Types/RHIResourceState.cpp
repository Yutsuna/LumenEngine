/**
 * @file RHIResourceState.cpp
 * @brief Implements resource-state masks and transition checks.
 */

#include <Types/RHIResourceState.hpp>

Lumen::Bool Lumen::IsValidRHIResourceState ( const ERHIResourceState InState ) noexcept
{
    constexpr UInt32 Mask = ( 1U << 15U ) - 1U;
    return ( static_cast<UInt32>( InState ) & ~Mask ) == 0U;
}

Lumen::Bool Lumen::IsValidRHIResourceTransition ( const ERHIResourceState InBefore, const ERHIResourceState InAfter ) noexcept
{
    if ( not IsValidRHIResourceState( InBefore ) or not IsValidRHIResourceState( InAfter ) )
    {
        return false;
    }
    return InBefore != InAfter;
}
