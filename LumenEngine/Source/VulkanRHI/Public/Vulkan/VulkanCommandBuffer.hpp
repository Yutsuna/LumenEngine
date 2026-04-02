/**
 * @file VulkanCommandBuffer.hpp
 * @brief Vulkan command buffer wrapper for recording rendering commands
 */

#pragma once

#include "Definitions.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FVulkanCommandBuffer
     * @brief Wrapper around VkCommandBuffer for recording rendering commands in Vulkan.
     * @friend FVulkanCommandPool to allow command pool to manage command buffer lifecycle.
     */
    class LUMEN_ENGINE_API FVulkanCommandBuffer
    {
    public:

        explicit FVulkanCommandBuffer ( VkCommandBuffer InHandle ) noexcept;

    public:

        /**
         * @brief Begins recording commands into the command buffer with the specified usage flags.
         * @param Flags Optional usage flags for command buffer recording (default is VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT).
         */
        void Begin ( VkCommandBufferUsageFlags Flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT );

        /** @brief Ends recording commands into the command buffer, making it ready for submission. */
        void End ();

        /**
         * @brief Inserts a pipeline barrier into the command buffer using the provided dependency information.
         * @param DepInfo A reference to a VkDependencyInfo structure that describes the memory and execution dependencies for the barrier.
         */
        void PipelineBarrier ( const VkDependencyInfo &DepInfo ) noexcept;

        /**
         * @brief Helper function to transition the layout of an image within the command buffer.
         * @param Image The Vulkan image whose layout is to be transitioned.
         * @param OldLayout The current layout of the image before the transition.
         * @param NewLayout The desired layout of the image after the transition.
         * @param AspectMask A bitmask specifying the aspect(s) of the image to transition (e.g., VK_IMAGE_ASPECT_COLOR_BIT).
         */
        void TransitionImageLayout ( VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout, VkImageAspectFlags AspectMask );

    public:

        [[nodiscard]] VkCommandBuffer GetHandle () const noexcept;

    private:

        friend class FVulkanCommandPool;
        VkCommandBuffer Handle = VK_NULL_HANDLE;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
