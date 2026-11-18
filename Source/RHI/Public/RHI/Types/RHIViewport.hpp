/**
 * @file RHIViewport.hpp
 * @brief Defines dynamic viewport state.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHI/Types/RHIExtent.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHIViewport
     * @brief Floating-point viewport and depth range.
     */
    struct FRHIViewport
    {
        Float32 X        = 0.0F;
        Float32 Y        = 0.0F;
        Float32 Width    = 0.0F;
        Float32 Height   = 0.0F;
        Float32 MinDepth = 0.0F;
        Float32 MaxDepth = 1.0F;

        /** @return True when dimensions are positive and depth is ordered within [0,1]. */
        [[nodiscard]] constexpr Bool IsValid () const noexcept
        {
            return Width > 0.0F and Height > 0.0F and MinDepth >= 0.0F and MaxDepth <= 1.0F and MinDepth <= MaxDepth;
        }
    };

} // namespace RHI

} // namespace Lumen
