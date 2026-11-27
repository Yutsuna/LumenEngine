/**
 * @file RHISwapchainCapabilities.cpp
 * @brief Implements swapchain capability validation and selection.
 */

#include <RHI/Presentation/RHISwapchainCapabilities.hpp>

#include <Algo/Any.hpp>

Lumen::RHI::FRHIExpected Lumen::RHI::FRHISwapchainCapabilities::Validate () const noexcept
{
    if ( Formats.empty() or ColorSpaces.empty() or PresentModes.empty() or not MinExtent.IsValid() or not MaxExtent.IsValid() or
         MinImageCount == 0U or MaxImageCount < MinImageCount or MaxExtent.Width < MinExtent.Width or
         MaxExtent.Height < MinExtent.Height or MaxExtent.Depth < MinExtent.Depth )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPresentation( "Invalid swapchain capability ranges" ) );
    }
    return {};
}

Lumen::Bool Lumen::RHI::FRHISwapchainCapabilities::Supports ( const ERHIFormat InFormat,
                                                              const ERHIColorSpace InColorSpace,
                                                              const ERHIPresentMode InPresentMode ) const noexcept
{
    return Algo::AnyOf( Formats,
                        [InFormat] ( const ERHIFormat Format )
    {
        return Format == InFormat;
    } ) and
           Algo::AnyOf( ColorSpaces,
                        [InColorSpace] ( const ERHIColorSpace ColorSpace )
    {
        return ColorSpace == InColorSpace;
    } ) and
           Algo::AnyOf( PresentModes, [InPresentMode] ( const ERHIPresentMode PresentMode )
    {
        return PresentMode == InPresentMode;
    } );
}

Lumen::RHI::FRHIExtent3D Lumen::RHI::FRHISwapchainCapabilities::ClampExtent ( const FRHIExtent3D &InExtent ) const noexcept
{
    return {
        Maths::Clamp( InExtent.Width, MinExtent.Width, MaxExtent.Width ),
        Maths::Clamp( InExtent.Height, MinExtent.Height, MaxExtent.Height ),
        Maths::Clamp( InExtent.Depth, MinExtent.Depth, MaxExtent.Depth ),
    };
}
