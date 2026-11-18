/**
 * @file RHIColorSpace.hpp
 * @brief Defines portable swapchain color spaces.
 */

#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHIColorSpace
     * @brief Color primaries and transfer function of presented images.
     */
    enum class ERHIColorSpace : UInt8
    {
        SrgbNonLinear,
        DisplayP3NonLinear,
        Hdr10St2084,
        ExtendedSrgbLinear,
        Count
    };

} // namespace RHI

} // namespace Lumen
