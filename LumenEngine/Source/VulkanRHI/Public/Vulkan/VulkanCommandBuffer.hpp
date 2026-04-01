/**
 * @file VulkanCommandBuffer
 * @brief Vulkan command buffer wrapper for recording rendering commands
 */

#pragma once

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    struct FVulkanCommandBuffer final
    {
        VkCommandBuffer Handle = VK_NULL_HANDLE;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
