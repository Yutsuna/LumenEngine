/**
 * @file RHISwapchainDesc.cpp
 * @brief Implements swapchain descriptor negotiation.
 */

#include <Presentation/RHISwapchainDesc.hpp>

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHISwapchainDesc::Validate () const noexcept
{
    const auto SurfaceValid = Surface.Validate();
    if ( not SurfaceValid )
    {
        return SurfaceValid;
    }
    if ( Format == ERHIFormat::Unknown or Format >= ERHIFormat::Count or ColorSpace >= ERHIColorSpace::Count or
         PresentMode >= ERHIPresentMode::Count or not Extent.IsValid() or ImageCount < 2U or
         not IsValidRHIResourceUsage( ImageUsage ) or not HasAnyFlags( ImageUsage, ERHIResourceUsage::RenderTarget ) or
         HasAnyFlags( ImageUsage, ERHIResourceUsage::DepthStencil ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPresentation( "Invalid swapchain format, extent, image count, or usage" ) );
    }
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHISwapchainDesc::Validate ( const FRHISwapchainCapabilities &InCapabilities ) const noexcept
{
    const auto DescValid = Validate();
    if ( not DescValid )
    {
        return DescValid;
    }
    const auto CapabilitiesValid = InCapabilities.Validate();
    if ( not CapabilitiesValid )
    {
        return CapabilitiesValid;
    }
    if ( not InCapabilities.Supports( Format, ColorSpace, PresentMode ) or ImageCount < InCapabilities.MinImageCount or
         ImageCount > InCapabilities.MaxImageCount or Extent.Width < InCapabilities.MinExtent.Width or
         Extent.Height < InCapabilities.MinExtent.Height or Extent.Depth < InCapabilities.MinExtent.Depth or
         Extent.Width > InCapabilities.MaxExtent.Width or Extent.Height > InCapabilities.MaxExtent.Height or
         Extent.Depth > InCapabilities.MaxExtent.Depth )
    {
        return TUnexpected<FRHIError>( MakeRHIUnsupported( Unsupported, ERHIErrorCondition::Presentation,
                                                           "Requested swapchain configuration is not supported" ) );
    }
    return {};
}
