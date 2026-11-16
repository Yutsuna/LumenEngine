/**
 * @file RHIInlineConstantRange.cpp
 * @brief Implements inline constant range validation.
 */

#include <Pipelines/RHIInlineConstantRange.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIInlineConstantRange::Validate () const noexcept
{
    if ( Size == 0U or Stages == ERHIShaderStage::None or Offset % 4U != 0U or Size % 4U != 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Invalid inline constant range" ) );
    }
    return {};
}
