/**
 * @file RHITextureDesc.cpp
 * @brief Implements texture descriptor validation.
 */
#include <Resources/RHITextureDesc.hpp>
#include <Types/RHISampleCount.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHITextureDesc::Validate () const noexcept
{
    if ( not Extent.IsValid() or Format == ERHIFormat::Unknown or Format >= ERHIFormat::Count or MipCount == 0U or
         LayerCount == 0U or ToSampleCount( SampleCount ) == 0U or not IsValidRHIResourceUsage( Usage ) or
         not IsValidRHIResourceState( InitialState ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Resource, "Invalid texture description" ) );
    }
    return {};
}
