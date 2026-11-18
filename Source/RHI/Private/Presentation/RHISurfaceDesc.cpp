/**
 * @file RHISurfaceDesc.cpp
 * @brief Implements backend-neutral surface descriptor validation.
 */

#include <Presentation/RHISurfaceDesc.hpp>

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHISurfaceDesc::Validate () const noexcept
{
    if ( Type == ERHISurfaceType::None or Type >= ERHISurfaceType::Count or NativeWindow == 0U or not InitialExtent.IsValid() or
         ( bRequiresDisplay and NativeDisplay == 0U ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPresentation( "Invalid surface payload or initial extent" ) );
    }
    return {};
}
