/**
 * @file RHIInlineConstantRange.cpp
 * @brief Implements inline constant range validation.
 */

#include <RHI/Pipelines/RHIInlineConstantRange.hpp>
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIInlineConstantRange::Validate () const noexcept
{
    if ( Size == 0U or Stages == ERHIShaderStage::None or Offset % 4U != 0U or Size % 4U != 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Invalid inline constant range" ) );
    }
    return {};
}
