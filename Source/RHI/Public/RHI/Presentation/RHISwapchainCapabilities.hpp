/**
 * @file RHISwapchainCapabilities.hpp
 * @brief Defines the formats, modes, and extents supported by a surface.
 */

#pragma once

#include <RHI/Presentation/RHIColorSpace.hpp>
#include <RHI/Presentation/RHIPresentMode.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIExtent.hpp>
#include <RHI/Types/RHIFormat.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHISwapchainCapabilities
     * @brief Immutable surface negotiation data.
     */
    struct RHI_EXPORT FRHISwapchainCapabilities
    {
        TVector<ERHIFormat> Formats;
        TVector<ERHIColorSpace> ColorSpaces;
        TVector<ERHIPresentMode> PresentModes;
        FRHIExtent3D MinExtent;
        FRHIExtent3D MaxExtent;
        UInt32 MinImageCount = 2U;
        UInt32 MaxImageCount = 8U;

        /** @return Success when the capability ranges and supported value lists are usable. */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
        /** @return True when the exact format, color space, and mode combination is advertised. */
        [[nodiscard]] Bool
        /**
         * @brief Tests whether an exact presentation combination is advertised.
         * @param InFormat Pixel format to test.
         * @param InColorSpace Color space to test.
         * @param InPresentMode Present mode to test.
         * @return True only when all three values are listed as supported.
         */
        Supports ( ERHIFormat InFormat, ERHIColorSpace InColorSpace, ERHIPresentMode InPresentMode ) const noexcept;
        /** @return InExtent clamped to the supported inclusive extent range. */
        [[nodiscard]] FRHIExtent3D ClampExtent ( const FRHIExtent3D &InExtent ) const noexcept;
    };

} // namespace RHI

} // namespace Lumen
