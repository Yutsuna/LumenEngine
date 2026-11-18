/**
 * @file RHIResourceUsage.cpp
 * @brief Implements resource usage mask validation.
 */
#include <RHI/Resources/RHIResourceUsage.hpp>
Lumen::Bool Lumen::RHI::IsValidRHIResourceUsage ( const ERHIResourceUsage InUsage ) noexcept
{
    constexpr UInt32 Mask = ( 1U << 11U ) - 1U;
    return ( static_cast<UInt32>( InUsage ) & ~Mask ) == 0U;
}
