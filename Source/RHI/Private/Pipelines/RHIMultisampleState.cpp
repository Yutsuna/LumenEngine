/**
 * @file RHIMultisampleState.cpp
 * @brief Implements multisample state validation.
 */

#include <RHI/Pipelines/RHIMultisampleState.hpp>
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIMultisampleState::Validate () const noexcept
{
    if ( ToSampleCount( Count ) == 0U or SampleMask == 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Invalid multisample state" ) );
    }
    return {};
}
