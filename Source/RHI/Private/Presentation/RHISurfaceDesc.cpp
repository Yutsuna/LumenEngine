/**
 * @file RHISurfaceDesc.cpp
 * @brief Implements native surface descriptor validation.
 */

#include <Presentation/RHISurfaceDesc.hpp>

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHISurfaceDesc::Validate () const noexcept
{
    const Bool bNeedsDisplay = Type == ERHISurfaceType::Xlib or Type == ERHISurfaceType::Xcb or Type == ERHISurfaceType::Wayland;
    if ( Type == ERHISurfaceType::None or Type >= ERHISurfaceType::Count or NativeWindow == 0U or not InitialExtent.IsValid() or
         ( bNeedsDisplay and NativeDisplay == 0U ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPresentation( "Invalid native surface payload or initial extent" ) );
    }
    return {};
}
