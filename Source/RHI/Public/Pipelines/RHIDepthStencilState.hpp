/**
 * @file RHIDepthStencilState.hpp
 * @brief Defines depth and stencil state.
 */

#pragma once
#include <RHIExport.hpp>
#include <Resources/RHISamplerDesc.hpp>
#include <Types/RHIError.hpp>
namespace Lumen
{
/**
 * @enum ERHIStencilOp
 * @brief Operations applied after stencil tests.
 */
enum class ERHIStencilOp : UInt8
{
    Keep,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap

};
/**
 * @struct FRHIStencilState
 * @brief Comparison and operations for one stencil face.
 */
struct FRHIStencilState
{
    ERHIStencilOp FailOp        = ERHIStencilOp::Keep;
    ERHIStencilOp DepthFailOp   = ERHIStencilOp::Keep;
    ERHIStencilOp PassOp        = ERHIStencilOp::Keep;
    ERHICompareFunction Compare = ERHICompareFunction::Always;
};
/**
 * @struct FRHIDepthStencilState
 * @brief Depth test/write and two-sided stencil state.
 */
struct RHI_EXPORT FRHIDepthStencilState
{
    Bool bDepthTest                  = false;
    Bool bDepthWrite                 = false;
    Bool bStencilTest                = false;
    ERHICompareFunction DepthCompare = ERHICompareFunction::LessEqual;
    FRHIStencilState Front;
    FRHIStencilState Back;
    /** @return Success for defined enum values. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
