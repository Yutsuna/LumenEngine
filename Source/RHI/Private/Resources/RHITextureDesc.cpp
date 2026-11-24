/**
 * @file RHITextureDesc.cpp
 * @brief Implements texture descriptor validation.
 */
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Types/RHISampleCount.hpp>
Lumen::RHI::FRHIExpected Lumen::RHI::FRHITextureDesc::Validate () const noexcept
{
    if ( not Extent.IsValid() or Format == ERHIFormat::Unknown or Format >= ERHIFormat::Count or MipCount == 0U or
         MipCount > Extent.GetMaxMipLevelCount() or LayerCount == 0U or ToSampleCount( SampleCount ) == 0U or
         not IsValidRHIResourceUsage( Usage ) or not IsValidRHIResourceState( InitialState ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidResource( "Invalid texture description" ) );
    }
    return {};
}
