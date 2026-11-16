/**
 * @file RHISwapchainDesc.hpp
 * @brief Defines swapchain creation and recreation parameters.
 */
#pragma once

#include <Memory/SharedPtr.hpp>
#include <Presentation/RHIColorSpace.hpp>
#include <Presentation/RHIPresentMode.hpp>
#include <Presentation/RHISurfaceDesc.hpp>
#include <Presentation/RHISwapchainCapabilities.hpp>
#include <RHIExport.hpp>
#include <Resources/RHIResourceUsage.hpp>

namespace Lumen
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
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    /** @return Success when every requested value is supported by InCapabilities. */
    [[nodiscard]] TExpected<void, FRHIError> Validate ( const FRHISwapchainCapabilities &InCapabilities ) const noexcept;
};

} // namespace Lumen
