/**
 * @file RHIRasterizerStateTests.cpp
 * @brief Tests rasterizer enum and finite bias validation.
 */

#include <Pipelines/RHIRasterizerState.hpp>
#include <Tests/UnitTest.hpp>
#include <limits>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIRasterizerState, RejectsInvalidEnumsAndNonFiniteBias )
    {
        FRHIRasterizerState State;
        LUMEN_CHECK_TRUE( State.Validate().HasValue() );
        State.Fill = static_cast<ERHIFillMode>( 3U );
        LUMEN_CHECK_HAS_ERROR( State.Validate() );
        State.Fill = ERHIFillMode::Solid;
        State.Cull = static_cast<ERHICullMode>( 3U );
        LUMEN_CHECK_HAS_ERROR( State.Validate() );
        State.Cull      = ERHICullMode::Back;
        State.FrontFace = static_cast<ERHIFrontFace>( 2U );
        LUMEN_CHECK_HAS_ERROR( State.Validate() );
        State.FrontFace         = ERHIFrontFace::CounterClockwise;
        State.DepthBiasConstant = std::numeric_limits<Float32>::infinity();
        LUMEN_CHECK_HAS_ERROR( State.Validate() );
        State.DepthBiasConstant = 0.0F;
        State.DepthBiasSlope    = std::numeric_limits<Float32>::quiet_NaN();
        LUMEN_CHECK_HAS_ERROR( State.Validate() );
    }

} // namespace
} // namespace Lumen
