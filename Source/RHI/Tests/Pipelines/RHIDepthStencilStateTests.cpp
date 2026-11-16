/**
 * @file RHIDepthStencilStateTests.cpp
 * @brief Tests depth-stencil enum validation.
 */

#include <Pipelines/RHIDepthStencilState.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIDepthStencilState, AcceptsDefaultAndTwoSidedStencil )
    {
        FRHIDepthStencilState State;
        LUMEN_CHECK_TRUE( State.Validate().HasValue() );
        State.bDepthTest   = true;
        State.bDepthWrite  = true;
        State.bStencilTest = true;
        State.Front.PassOp = ERHIStencilOp::Replace;
        State.Back.Compare = ERHICompareFunction::Equal;
        LUMEN_CHECK_TRUE( State.Validate().HasValue() );
    }

    LUMEN_TEST( RHIDepthStencilState, RejectsEveryInvalidOperationAndComparison )
    {
        FRHIDepthStencilState State;
        State.DepthCompare = static_cast<ERHICompareFunction>( 8U );
        LUMEN_CHECK_HAS_ERROR( State.Validate() );
        State.DepthCompare = ERHICompareFunction::LessEqual;
        State.Front.FailOp = static_cast<ERHIStencilOp>( 8U );
        LUMEN_CHECK_HAS_ERROR( State.Validate() );
        State.Front.FailOp      = ERHIStencilOp::Keep;
        State.Front.DepthFailOp = static_cast<ERHIStencilOp>( 255U );
        LUMEN_CHECK_HAS_ERROR( State.Validate() );
        State.Front.DepthFailOp = ERHIStencilOp::Keep;
        State.Back.PassOp       = static_cast<ERHIStencilOp>( 8U );
        LUMEN_CHECK_HAS_ERROR( State.Validate() );
        State.Back.PassOp  = ERHIStencilOp::Keep;
        State.Back.Compare = static_cast<ERHICompareFunction>( 8U );
        LUMEN_CHECK_HAS_ERROR( State.Validate() );
    }

} // namespace
} // namespace Lumen
