/**
 * @file RHIResourceUsage.cpp
 * @brief Implements resource usage mask validation.
 */
#include <Resources/RHIResourceUsage.hpp>
Lumen::Bool Lumen::IsValidRHIResourceUsage ( const ERHIResourceUsage InUsage ) noexcept
{
    constexpr UInt32 Mask = ( 1U << 11U ) - 1U;
    return ( static_cast<UInt32>( InUsage ) & ~Mask ) == 0U;
}
