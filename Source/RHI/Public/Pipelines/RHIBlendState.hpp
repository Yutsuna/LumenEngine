/**
 * @file RHIBlendState.hpp
 * @brief Defines color blending state.
 */

#pragma once

#include <RHIExport.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHIError.hpp>

namespace Lumen
{

/**
 * @enum ERHIBlendFactor
 * @brief Source/destination blend factors.
 */
enum class ERHIBlendFactor : UInt8
{
    Zero,
    One,
    SourceAlpha,
    InverseSourceAlpha,
    DestinationColor,
    InverseDestinationColor
};
/**
 * @enum ERHIBlendOp
 * @brief Color/alpha blend operations.
 */
enum class ERHIBlendOp : UInt8
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max

};
/**
 * @struct FRHIBlendAttachmentState
 * @brief Per-attachment blending and channel mask.
 */
struct FRHIBlendAttachmentState
{
    Bool bEnabled                    = false;
    ERHIBlendFactor SourceColor      = ERHIBlendFactor::One;
    ERHIBlendFactor DestinationColor = ERHIBlendFactor::Zero;
    ERHIBlendOp ColorOp              = ERHIBlendOp::Add;
    ERHIBlendFactor SourceAlpha      = ERHIBlendFactor::One;
    ERHIBlendFactor DestinationAlpha = ERHIBlendFactor::Zero;
    ERHIBlendOp AlphaOp              = ERHIBlendOp::Add;
    UInt8 WriteMask                  = 0xFU;
};
/**
 * @struct FRHIBlendState
 * @brief Blend state for all color attachments.
 */
struct RHI_EXPORT FRHIBlendState
{
    TVector<FRHIBlendAttachmentState> Attachments;
    /** @return Success when each attachment has a valid mask and enum values. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
