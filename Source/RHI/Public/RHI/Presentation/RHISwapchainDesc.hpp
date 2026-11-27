/**
 * @file RHISwapchainDesc.hpp
 * @brief Defines swapchain creation and recreation parameters.
 */

#pragma once

#include <Memory/SharedPtr.hpp>
#include <RHI/Presentation/RHIColorSpace.hpp>
#include <RHI/Presentation/RHIPresentMode.hpp>
#include <RHI/Presentation/RHISurfaceDesc.hpp>
#include <RHI/Presentation/RHISwapchainCapabilities.hpp>
#include <RHI/Resources/RHIResourceUsage.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    class IRHISwapchain;

    /**
     * @struct FRHISwapchainDesc
     * @brief Immutable swapchain image and presentation configuration.
     */
    struct RHI_EXPORT FRHISwapchainDesc
    {
        FRHISurfaceDesc Surface;
        ERHIFormat Format           = ERHIFormat::RGBA8Unorm;
        ERHIColorSpace ColorSpace   = ERHIColorSpace::SrgbNonLinear;
        ERHIPresentMode PresentMode = ERHIPresentMode::Fifo;
        FRHIExtent3D Extent;
        UInt32 ImageCount            = 3U;
        ERHIResourceUsage ImageUsage = ERHIResourceUsage::RenderTarget;
        TSharedPtr<IRHISwapchain> OldSwapchain;

        /** @return Success when the descriptor is internally coherent. */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
        /** @return Success when every requested value is supported by InCapabilities. */
        [[nodiscard]] FRHIExpected Validate ( const FRHISwapchainCapabilities &InCapabilities ) const noexcept;
    };

} // namespace RHI

} // namespace Lumen
