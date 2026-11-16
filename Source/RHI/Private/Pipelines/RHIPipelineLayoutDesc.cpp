/**
 * @file RHIPipelineLayoutDesc.cpp
 * @brief Implements pipeline layout validation.
 */

#include <Pipelines/RHIPipelineLayoutDesc.hpp>
#include <Utilities/RHICheckedMath.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIPipelineLayoutDesc::Validate () const noexcept
{
    for ( USize First = 0U; First < InlineConstants.size(); ++First )
    {
        if ( not InlineConstants[First].Validate() )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Invalid inline range" ) );
        }
        const auto End = RHICheckedAdd( InlineConstants[First].Offset, InlineConstants[First].Size );
        if ( not End )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Inline range overflow" ) );
        }
        for ( USize Second = First + 1U; Second < InlineConstants.size(); ++Second )
        {
            const auto OtherEnd = RHICheckedAdd( InlineConstants[Second].Offset, InlineConstants[Second].Size );
            if ( OtherEnd and InlineConstants[First].Offset < *OtherEnd and InlineConstants[Second].Offset < *End )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Inline ranges overlap" ) );
            }
        }
    }
    return {};
}
