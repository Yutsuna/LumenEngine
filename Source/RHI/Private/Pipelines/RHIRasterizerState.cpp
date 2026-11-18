/**
 * @file RHIRasterizerState.cpp
 * @brief Implements rasterizer state validation.
 */

#include <RHI/Pipelines/RHIRasterizerState.hpp>
#include <cmath>
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIRasterizerState::Validate () const noexcept
{
    if ( Fill > ERHIFillMode::Point or Cull > ERHICullMode::Back or FrontFace > ERHIFrontFace::Clockwise or
         not std::isfinite( DepthBiasConstant ) or not std::isfinite( DepthBiasSlope ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Invalid rasterizer state" ) );
    }
    return {};
}
