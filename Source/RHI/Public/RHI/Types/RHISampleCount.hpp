/**
 * @file RHISampleCount.hpp
 * @brief Defines supported multisample counts.
 */
#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHISampleCount
     * @brief Power-of-two sample counts exposed by the RHI.
     */
    enum class ERHISampleCount : UInt8
    {
        Count1  = 1U,
        Count2  = 2U,
        Count4  = 4U,
        Count8  = 8U,
        Count16 = 16U,
        Count32 = 32U,
        Count64 = 64U,
    };

    /** @return Numeric sample count, or zero for an invalid enum value. */
    [[nodiscard]] constexpr UInt32 ToSampleCount ( ERHISampleCount InCount ) noexcept
    {
        const UInt32 Value = static_cast<UInt32>( InCount );
        return ( Value != 0U and ( Value & ( Value - 1U ) ) == 0U and Value <= 64U ) ? Value : 0U;
    }

} // namespace RHI

} // namespace Lumen
