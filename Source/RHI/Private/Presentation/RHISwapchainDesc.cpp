/**
 * @file RHISwapchainDesc.cpp
 * @brief Implements swapchain descriptor negotiation.
 */

#include <RHI/Presentation/RHISwapchainDesc.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIExpected Lumen::RHI::FRHISwapchainDesc::Validate () const noexcept
{
    return Surface.Validate().AndThen( [this] () noexcept -> FRHIExpected
    {
        if ( Format == ERHIFormat::Unknown or Format >= ERHIFormat::Count or ColorSpace >= ERHIColorSpace::Count or
             PresentMode >= ERHIPresentMode::Count or not Extent.IsValid() or ImageCount < 2U or
             not IsValidRHIResourceUsage( ImageUsage ) or not HasAnyFlags( ImageUsage, ERHIResourceUsage::RenderTarget ) or
             HasAnyFlags( ImageUsage, ERHIResourceUsage::DepthStencil ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidPresentation( "Invalid swapchain format, extent, image count, or usage" ) );
        }
        return {};
    } );
}

Lumen::RHI::FRHIExpected
Lumen::RHI::FRHISwapchainDesc::Validate ( const FRHISwapchainCapabilities &InCapabilities ) const noexcept
{
    return Validate()
        .AndThen(
            [&InCapabilities] () noexcept -> FRHIExpected
    {
        return InCapabilities.Validate();
    } )
        .AndThen( [this, &InCapabilities] () noexcept -> FRHIExpected
    {
        if ( not InCapabilities.Supports( Format, ColorSpace, PresentMode ) or ImageCount < InCapabilities.MinImageCount or
             ImageCount > InCapabilities.MaxImageCount or Extent.Width < InCapabilities.MinExtent.Width or
             Extent.Height < InCapabilities.MinExtent.Height or Extent.Depth < InCapabilities.MinExtent.Depth or
             Extent.Width > InCapabilities.MaxExtent.Width or Extent.Height > InCapabilities.MaxExtent.Height or
             Extent.Depth > InCapabilities.MaxExtent.Depth )
        {
            return TUnexpected<FRHIError>(
                MakeRHIUnsupported( ERHIErrorCondition::Presentation, "Requested swapchain configuration is not supported" ) );
        }
        return {};
    } );
}
