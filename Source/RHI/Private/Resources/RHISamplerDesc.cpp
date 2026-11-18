/**
 * @file RHISamplerDesc.cpp
 * @brief Implements sampler descriptor validation.
 */
#include <RHI/Resources/RHISamplerDesc.hpp>
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHISamplerDesc::Validate () const noexcept
{
    if ( MinLod < 0.0F or MaxLod < MinLod or MaxAnisotropy == 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidResource( "Invalid sampler LOD or anisotropy" ) );
    }
    return {};
}
