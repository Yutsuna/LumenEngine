/**
 * @file RHISamplerDesc.cpp
 * @brief Implements sampler descriptor validation.
 */
#include <Resources/RHISamplerDesc.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHISamplerDesc::Validate () const noexcept
{
    if ( MinLod < 0.0F or MaxLod < MinLod or MaxAnisotropy == 0U )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Resource, "Invalid sampler LOD or anisotropy" ) );
    }
    return {};
}
