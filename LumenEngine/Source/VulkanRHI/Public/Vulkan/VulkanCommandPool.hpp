/**
 * @file VulkanCommandPool.hpp
 * @brief Vulkan command pool wrapper for managing command buffers
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    class FVulkanCommandBuffer;

    /**
     * @class FVulkanCommandPool
     * @brief Wrapper around VkCommandPool for managing command buffers in Vulkan.
     */
    class LUMEN_ENGINE_API FVulkanCommandPool
    {

    public:

        FVulkanCommandPool () noexcept  = default;
        ~FVulkanCommandPool () noexcept = default;

    public:

        /**
         * @brief Initializes the command pool with the specified device and queue family index.
         * @param Device The Vulkan logical device to create the command pool for.
         * @param QueueFamilyIndex The index of the queue family that the command buffers allocated from this pool will be submitted to.
         * @param Flags Optional flags for command pool creation (default is VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT).
         */
        void Initialize ( VkDevice Device, UInt32 QueueFamilyIndex, VkCommandPoolCreateFlags Flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );

        /** @brief Cleans up the command pool by destroying the Vulkan command pool handle. */
        void Cleanup ( VkDevice Device ) noexcept;

        /**
         * @brief Allocates a command buffer from the command pool.
         * @param Device The Vulkan logical device to allocate the command buffer from.
         * @param Level The level of the command buffer (primary or secondary). Default is VK_COMMAND_BUFFER_LEVEL_PRIMARY.
         * @return An FVulkanCommandBuffer wrapper around the allocated VkCommandBuffer handle.
         */
        [[nodiscard]] FVulkanCommandBuffer AllocateBuffer ( VkDevice Device, VkCommandBufferLevel Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY );

        /**
         * @brief Resets the command pool, which implicitly resets all command buffers allocated from it.
         * @param Device The Vulkan logical device associated with the command pool.
         */
        void Reset ( VkDevice Device );

        [[nodiscard]] VkCommandPool GetHandle () const noexcept;

    private:

        VkCommandPool Handle = VK_NULL_HANDLE;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
