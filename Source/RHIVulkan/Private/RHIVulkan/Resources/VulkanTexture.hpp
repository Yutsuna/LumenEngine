/**
 * @file VulkanTexture.hpp
 * @brief Declares a Vulkan image resource with dedicated or placed memory.
 */
#pragma once

#include <Memory/SharedPtr.hpp>

#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Memory/RHIResourceRequirements.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

#include <RHIVulkan/Device/VulkanDeviceContext.hpp>
#include <RHIVulkan/Types/VulkanHandle.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class FVulkanTexture
     * @brief Native VkImage with a dedicated allocation or a retained placed heap range.
     */
    class FVulkanTexture final : public RHI::IRHITexture
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /**
         * @brief Creates an image and binds a native allocation selected for its memory class.
         * @param InContext Shared native device context.
         * @param InDesc Validated texture descriptor.
         * @param InId Identifier assigned by the owning device.
         * @return The texture, or a structured error with the Resource/Memory/Limit condition.
         */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanTexture>, RHI::FRHIError> Create (
            TSharedRef<FVulkanDeviceContext> InContext, const RHI::FRHITextureDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        /**
         * @brief Creates an image placed in an existing heap range, retaining the heap.
         * @param InContext Shared native device context.
         * @param InDesc Validated texture descriptor.
         * @param InHeap Heap owning the native memory.
         * @param InAllocation Validated range within the heap.
         * @param InId Identifier assigned by the owning device.
         * @return The texture, or a structured error with the Resource/Memory condition.
         */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanTexture>, RHI::FRHIError>
        CreatePlaced ( TSharedRef<FVulkanDeviceContext> InContext,
                       const RHI::FRHITextureDesc &InDesc,
                       TSharedRef<RHI::IRHIHeap> InHeap,
                       const RHI::FRHIAllocation &InAllocation,
                       RHI::FRHIObjectId InId ) noexcept;

        ~FVulkanTexture () noexcept override;

        const RHI::FRHITextureDesc &GetDesc () const noexcept override;
        const RHI::FRHIResourceRequirements &GetRequirements () const noexcept override;
        RHI::ERHIResourceUsage GetUsage () const noexcept override;
        RHI::ERHIResourceState GetInitialState () const noexcept override;
        RHI::FRHIResourceLifetime GetLifetime () const noexcept override;

        /** @return Native VkImage handle used by views and later command translation. */
        [[nodiscard]] VkImage GetNativeImage () const noexcept;

        FVulkanTexture ( TSharedRef<FVulkanDeviceContext> InContext,
                         const RHI::FRHITextureDesc &InDesc,
                         RHI::FRHIObjectId InId ) noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FVulkanDeviceContext> Context;
        RHI::FRHITextureDesc Desc;
        RHI::FRHIResourceRequirements Requirements;
        FVulkanImageHandle Image;
        FVulkanDeviceMemoryHandle OwnMemory;
        TSharedPtr<RHI::IRHIHeap> Heap;
        RHI::FRHIAllocation Allocation;
    };

} // namespace RHIVulkan

} // namespace Lumen
