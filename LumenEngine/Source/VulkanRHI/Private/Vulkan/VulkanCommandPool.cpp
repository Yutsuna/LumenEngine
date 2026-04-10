/**
 * @file VulkanCommandPool.cpp
 * @brief Vulkan command pool wrapper for managing command buffers
 */

#include "Vulkan/VulkanCommandPool.hpp"
#include "Vulkan/VulkanCommandBuffer.hpp"
#include "Vulkan/VulkanCore.hpp"

void LumenEngine::VulkanRHI::FVulkanCommandPool::Initialize ( VkDevice Device, UInt32 QueueFamilyIndex, VkCommandPoolCreateFlags Flags )
{
    VkCommandPoolCreateInfo PoolInfo{};
    PoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolInfo.flags            = Flags;
    PoolInfo.queueFamilyIndex = QueueFamilyIndex;

    LUMEN_VK_CHECK( vkCreateCommandPool( Device, &PoolInfo, nullptr, &Handle ) );
}

void LumenEngine::VulkanRHI::FVulkanCommandPool::Cleanup ( VkDevice Device ) noexcept
{
    if ( Handle != VK_NULL_HANDLE )
    {
        vkDestroyCommandPool( Device, Handle, nullptr );
        Handle = VK_NULL_HANDLE;
    }
}

LumenEngine::VulkanRHI::FVulkanCommandBuffer LumenEngine::VulkanRHI::FVulkanCommandPool::AllocateBuffer ( VkDevice Device, VkCommandBufferLevel Level )
{
    VkCommandBufferAllocateInfo AllocInfo{};
    AllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocInfo.commandPool        = Handle;
    AllocInfo.level              = Level;
    AllocInfo.commandBufferCount = 1;

    VkCommandBuffer RawBuffer{};
    LUMEN_VK_CHECK( vkAllocateCommandBuffers( Device, &AllocInfo, &RawBuffer ) );

    return FVulkanCommandBuffer{ RawBuffer };
}

void LumenEngine::VulkanRHI::FVulkanCommandPool::Reset ( VkDevice Device )
{
    LUMEN_VK_CHECK( vkResetCommandPool( Device, Handle, 0 ) );
}

VkCommandPool LumenEngine::VulkanRHI::FVulkanCommandPool::GetHandle () const noexcept
{
    return Handle;
}
