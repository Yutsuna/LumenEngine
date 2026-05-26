/**
 * @file VulkanMsaaRenderTarget.hpp
 * @brief Declaration of the FVulkanMsaaRenderTarget class for transient MSAA targets.
 */

#pragma once

#include "Definitions.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FVulkanMsaaRenderTarget
     * @brief Encapsulates transient multisampled color buffer creation, layout, and lifetime for resolves.
     */
    class LUMEN_ENGINE_API FVulkanMsaaRenderTarget final
    {
    public:

        FVulkanMsaaRenderTarget () noexcept  = default;
        ~FVulkanMsaaRenderTarget () noexcept = default;

    public:

        /**
         * @brief Allocates the multisampled image and creates its associated image view.
         * @param InAllocator The VMA Allocator handle.
         * @param InDevice The Vulkan logical device.
         * @param InFormat The color format.
         * @param InExtent Target size of the viewport.
         * @param InSamples The number of MSAA samples.
         */
        void Create ( VmaAllocator InAllocator, VkDevice InDevice, VkFormat InFormat, VkExtent2D InExtent, VkSampleCountFlagBits InSamples );

        /**
         * @brief Cleans up and destroys all Vulkan resources associated with this render target.
         * @param InAllocator The VMA Allocator handle.
         * @param InDevice The Vulkan logical device.
         */
        void Destroy ( VmaAllocator InAllocator, VkDevice InDevice ) noexcept;

        /**
         * @brief Checks if any parameters changed, and recreates the target only if necessary.
         */
        void RecreateIfNeeded ( VmaAllocator InAllocator, VkDevice InDevice, VkFormat InFormat, VkExtent2D InExtent, VkSampleCountFlagBits InSamples );

        /**
         * @brief Releases ownership of internal handles to allow deferred destruction.
         */
        void ReleaseOwnership ( VkImage &OutImage, VkImageView &OutView, VmaAllocation &OutAllocation ) noexcept;

    public:

        [[nodiscard]] VkImage GetImage () const noexcept;
        [[nodiscard]] VkImageView GetImageView () const noexcept;
        [[nodiscard]] VkSampleCountFlagBits GetSamples () const noexcept;

    private:

        VkImage Image                 = VK_NULL_HANDLE;
        VkImageView ImageView         = VK_NULL_HANDLE;
        VmaAllocation Allocation      = VK_NULL_HANDLE;
        VkFormat Format               = VK_FORMAT_UNDEFINED;
        VkExtent2D Extent             = {};
        VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
