/**
 * @file RHIComputePipelineDesc.cpp
 * @brief Implements compute pipeline validation.
 */

#include <RHI/Pipelines/RHIComputePipelineDesc.hpp>
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIComputePipelineDesc::Validate () const noexcept
{
    if ( not Shader or not Layout or Shader->GetDesc().Stage != ERHIShaderStage::Compute )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Compute pipeline requires compute shader and layout" ) );
    }
    return {};
}
