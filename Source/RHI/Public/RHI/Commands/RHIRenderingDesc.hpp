/**
 * @file RHIRenderingDesc.hpp
 * @brief Defines dynamic-rendering attachments and scope.
 */

#pragma once
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Types/RHIClearValue.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>
namespace Lumen
{

namespace RHI
{
    /**
     * @enum ERHIRenderingLoadOp
     * @brief Initial-content action for a rendering attachment.
     */
    enum class ERHIRenderingLoadOp : UInt8
    {
        Load,
        Clear,
        DontCare
    };
    /**
     * @enum ERHIRenderingStoreOp
     * @brief Final-content action for a rendering attachment.
     */
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
        /**
         * @brief Validates an attachment for its color or depth/stencil role.
         * @param bDepthStencil True when the attachment is a depth/stencil target.
         * @return Success, or a structured FRHIError with the relevant condition.
         */
        [[nodiscard]] FRHIExpected Validate ( Bool bDepthStencil ) const noexcept;
    };
    struct RHI_EXPORT FRHIRenderingDesc
    {
        FRHIRect2D Area;
        TVector<FRHIRenderingAttachment> ColorAttachments;
        FRHIRenderingAttachment DepthStencilAttachment;
        /**
         * @brief Validates the rendering area and all attachments.
         * @return Success, or a structured FRHIError with the relevant condition.
         */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };
} // namespace RHI

} // namespace Lumen
