/**
 * @file VulkanCommandPool.hpp
 * @brief Vulkan command pool wrapper for managing command buffers
 */

#pragma once

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    struct FVulkanCommandPool final
    {
        VkCommandPool Handle = VK_NULL_HANDLE;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
