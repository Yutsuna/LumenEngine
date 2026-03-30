/**
 * @file VulkanBuffer.hpp
 * @brief Vulkan buffer wrapper for resource management
 */

#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    struct FVulkanBuffer
    {
        VkBuffer Buffer                  = VK_NULL_HANDLE;
        VmaAllocation Allocation         = VK_NULL_HANDLE;
        VmaAllocationInfo AllocationInfo = {};
        VkDeviceAddress DeviceAddress    = 0;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
