/**
 * @file RHISurfaceDesc.cpp
 * @brief Implements backend-neutral surface descriptor validation.
 */

#include <RHI/Presentation/RHISurfaceDesc.hpp>

Lumen::RHI::FRHIExpected Lumen::RHI::FRHISurfaceDesc::Validate () const noexcept
{
    if ( Type == ERHISurfaceType::None or Type >= ERHISurfaceType::Count or NativeWindow == 0U or not InitialExtent.IsValid() or
         ( bRequiresDisplay and NativeDisplay == 0U ) or ( Type == ERHISurfaceType::Window and not Bridge ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPresentation( "Invalid surface payload or initial extent" ) );
    }
    return {};
}
