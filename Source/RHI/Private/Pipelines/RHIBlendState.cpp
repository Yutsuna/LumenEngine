/**
 * @file RHIBlendState.cpp
 * @brief Implements blend state validation.
 */

#include <Pipelines/RHIBlendState.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIBlendState::Validate () const noexcept
{
    for ( const auto &Attachment : Attachments )
    {
        if ( Attachment.WriteMask > 0xFU or Attachment.ColorOp > ERHIBlendOp::Max or Attachment.AlphaOp > ERHIBlendOp::Max or
             Attachment.SourceColor > ERHIBlendFactor::InverseDestinationColor or
             Attachment.DestinationColor > ERHIBlendFactor::InverseDestinationColor or
             Attachment.SourceAlpha > ERHIBlendFactor::InverseDestinationColor or
             Attachment.DestinationAlpha > ERHIBlendFactor::InverseDestinationColor )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Invalid blend attachment" ) );
        }
    }
    return {};
}
