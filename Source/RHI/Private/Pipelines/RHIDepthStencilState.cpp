/**
 * @file RHIDepthStencilState.cpp
 * @brief Implements depth-stencil state validation.
 */

#include <RHI/Pipelines/RHIDepthStencilState.hpp>
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIDepthStencilState::Validate () const noexcept
{
    if ( DepthCompare > ERHICompareFunction::Always or Front.Compare > ERHICompareFunction::Always or
         Back.Compare > ERHICompareFunction::Always or Front.FailOp > ERHIStencilOp::DecrementWrap or
         Front.DepthFailOp > ERHIStencilOp::DecrementWrap or Front.PassOp > ERHIStencilOp::DecrementWrap or
         Back.FailOp > ERHIStencilOp::DecrementWrap or Back.DepthFailOp > ERHIStencilOp::DecrementWrap or
         Back.PassOp > ERHIStencilOp::DecrementWrap )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Invalid depth-stencil state" ) );
    }
    return {};
}
