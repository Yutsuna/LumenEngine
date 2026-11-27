/**
 * @file VulkanTextureView.hpp
 * @brief Declares a Vulkan image view retaining its parent texture.
 */
#pragma once

#include <Memory/SharedPtr.hpp>

#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

#include <RHIVulkan/Device/VulkanDeviceContext.hpp>
#include <RHIVulkan/Types/VulkanHandle.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class FVulkanTextureView
     * @brief Native VkImageView over a validated subresource range of a retained texture.
     */
    class FVulkanTextureView final : public RHI::IRHITextureView
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /**
         * @brief Creates a view after validating the descriptor and the parent resource.
         * @param InContext Shared native device context.
         * @param InDesc Validated view descriptor with a Vulkan parent texture.
         * @param InId Identifier assigned by the owning device.
         * @return The view, or a structured error with the Resource condition.
         */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanTextureView>, RHI::FRHIError> Create (
            TSharedRef<FVulkanDeviceContext> InContext, const RHI::FRHITextureViewDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        ~FVulkanTextureView () noexcept override;

        const RHI::FRHITextureViewDesc &GetDesc () const noexcept override;
        TSharedPtr<RHI::IRHITexture> GetTexture () const noexcept override;

        FVulkanTextureView ( TSharedRef<FVulkanDeviceContext> InContext,
                             const RHI::FRHITextureViewDesc &InDesc,
                             TSharedPtr<RHI::IRHITexture> InTexture,
                             RHI::FRHIObjectId InId ) noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FVulkanDeviceContext> Context;
        RHI::FRHITextureViewDesc Desc;
        TSharedPtr<RHI::IRHITexture> Texture;
        FVulkanImageViewHandle View;
    };

} // namespace RHIVulkan

} // namespace Lumen
