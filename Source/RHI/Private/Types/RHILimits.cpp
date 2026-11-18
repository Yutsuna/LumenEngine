/**
 * @file RHILimits.cpp
 * @brief Implements device-limit coherence checks.
 */

#include <RHI/Types/RHILimits.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHILimits::Validate () const noexcept
{
    const Bool bValid = MaxBufferSize > 0ULL and MaxTextureDimension1D > 0U and MaxTextureDimension2D > 0U and
                        MaxTextureDimension3D > 0U and MaxTextureArrayLayers > 0U and MaxColorAttachments > 0U and
                        MaxBindingSets > 0U and MaxDescriptorsPerSet > 0U and MaxVertexAttributes > 0U and
                        MaxVertexBuffers > 0U and MaxComputeWorkgroupSize > 0U and TimestampPeriod > 0ULL and
                        RHIIsPowerOfTwo( MinBufferAlignment ) and RHIIsPowerOfTwo( MinTextureRowAlignment );
    if ( not bValid )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Limit, "RHI limits must be nonzero and alignments powers of two" ) );
    }
    return {};
}
