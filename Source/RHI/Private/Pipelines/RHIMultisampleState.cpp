/**
 * @file RHIMultisampleState.cpp
 * @brief Implements multisample state validation.
 */

#include <Pipelines/RHIMultisampleState.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIMultisampleState::Validate () const noexcept
{
    if ( ToSampleCount( Count ) == 0U or SampleMask == 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Invalid multisample state" ) );
    }
    return {};
}
