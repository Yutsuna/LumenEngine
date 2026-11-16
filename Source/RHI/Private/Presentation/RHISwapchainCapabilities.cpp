/**
 * @file RHISwapchainCapabilities.cpp
 * @brief Implements swapchain capability validation and selection.
 */

#include <Presentation/RHISwapchainCapabilities.hpp>

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHISwapchainCapabilities::Validate () const noexcept
{
    if ( Formats.empty() or ColorSpaces.empty() or PresentModes.empty() or not MinExtent.IsValid() or not MaxExtent.IsValid() or
         MinImageCount == 0U or MaxImageCount < MinImageCount or MaxExtent.Width < MinExtent.Width or
         MaxExtent.Height < MinExtent.Height or MaxExtent.Depth < MinExtent.Depth )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Presentation, "Invalid swapchain capability ranges" ) );
    }
    return {};
}

Lumen::Bool Lumen::FRHISwapchainCapabilities::Supports ( const ERHIFormat InFormat,
                                                         const ERHIColorSpace InColorSpace,
                                                         const ERHIPresentMode InPresentMode ) const noexcept
{
    Bool bHasFormat      = false;
    Bool bHasColorSpace  = false;
    Bool bHasPresentMode = false;
    for ( const ERHIFormat Format : Formats )
    {
        bHasFormat = bHasFormat or Format == InFormat;
    }
    for ( const ERHIColorSpace ColorSpace : ColorSpaces )
    {
        bHasColorSpace = bHasColorSpace or ColorSpace == InColorSpace;
    }
    for ( const ERHIPresentMode PresentMode : PresentModes )
    {
        bHasPresentMode = bHasPresentMode or PresentMode == InPresentMode;
    }
    return bHasFormat and bHasColorSpace and bHasPresentMode;
}

Lumen::FRHIExtent3D Lumen::FRHISwapchainCapabilities::ClampExtent ( const FRHIExtent3D &InExtent ) const noexcept
{
    FRHIExtent3D Result = InExtent;
    Result.Width        = Result.Width < MinExtent.Width ? MinExtent.Width : Result.Width;
    Result.Width        = Result.Width > MaxExtent.Width ? MaxExtent.Width : Result.Width;
    Result.Height       = Result.Height < MinExtent.Height ? MinExtent.Height : Result.Height;
    Result.Height       = Result.Height > MaxExtent.Height ? MaxExtent.Height : Result.Height;
    Result.Depth        = Result.Depth < MinExtent.Depth ? MinExtent.Depth : Result.Depth;
    Result.Depth        = Result.Depth > MaxExtent.Depth ? MaxExtent.Depth : Result.Depth;
    return Result;
}
