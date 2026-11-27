/**
 * @file RHIPipelineLayoutDesc.cpp
 * @brief Implements pipeline layout validation.
 */

#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIPipelineLayoutDesc::Validate () const noexcept
{
    for ( USize First = 0U; First < InlineConstants.size(); ++First )
    {
        if ( not InlineConstants[First].Validate() )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Invalid inline range" ) );
        }
        const TOptional<UInt32> End = RHICheckedAdd( InlineConstants[First].Offset, InlineConstants[First].Size );
        if ( not End )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Inline range overflow" ) );
        }
        for ( USize Second = First + 1U; Second < InlineConstants.size(); ++Second )
        {
            const TOptional<UInt32> OtherEnd = RHICheckedAdd( InlineConstants[Second].Offset, InlineConstants[Second].Size );
            if ( OtherEnd and InlineConstants[First].Offset < *OtherEnd and InlineConstants[Second].Offset < *End )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Inline ranges overlap" ) );
            }
        }
    }
    return {};
}
