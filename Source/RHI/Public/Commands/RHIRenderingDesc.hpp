/**
 * @file RHIRenderingDesc.hpp
 * @brief Defines dynamic-rendering attachments and scope.
 */

#pragma once
#include <Resources/IRHITextureView.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHIClearValue.hpp>
#include <Types/RHIError.hpp>
namespace Lumen
{
enum class ERHIRenderingLoadOp : UInt8
{
    Load,
    Clear,
    DontCare
};
enum class ERHIRenderingStoreOp : UInt8
{
    Store,
    DontCare
};
struct RHI_EXPORT FRHIRenderingAttachment
{
    TSharedPtr<IRHITextureView> View;
    ERHIRenderingLoadOp LoadOp   = ERHIRenderingLoadOp::Load;
    ERHIRenderingStoreOp StoreOp = ERHIRenderingStoreOp::Store;
    FRHIClearValue ClearValue;
    [[nodiscard]] TExpected<void, FRHIError> Validate ( Bool bDepthStencil ) const noexcept;
};
struct RHI_EXPORT FRHIRenderingDesc
{
    FRHIRect2D Area;
    TVector<FRHIRenderingAttachment> ColorAttachments;
    FRHIRenderingAttachment DepthStencilAttachment;
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
