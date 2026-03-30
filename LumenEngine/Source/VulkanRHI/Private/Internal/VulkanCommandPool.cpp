#include "VulkanCommandPool.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanCore.hpp"
#include "VulkanDevice.hpp"

LumenEngine::RHI::FVulkanCommandPool::FVulkanCommandPool ( FVulkanDevice &InDevice, UInt32 InQueueFamilyIndex ) noexcept
    : Device( InDevice ), QueueFamilyIndex( InQueueFamilyIndex )
{
    VkCommandPoolCreateInfo PoolInfo{};
    PoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolInfo.queueFamilyIndex = QueueFamilyIndex;
    // Allow individual command buffers to be reset, useful for multi-threading
    PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VULKAN_CHECK( vkCreateCommandPool( Device.GetLogicalDevice(), &PoolInfo, nullptr, &Handle ) );
}

LumenEngine::RHI::FVulkanCommandPool::~FVulkanCommandPool () noexcept
{
    if ( Handle != VK_NULL_HANDLE )
    {
        vkDestroyCommandPool( Device.GetLogicalDevice(), Handle, nullptr );
        Handle = VK_NULL_HANDLE;
    }
}

LumenEngine::TUniquePtr<LumenEngine::RHI::FVulkanCommandBuffer> LumenEngine::RHI::FVulkanCommandPool::AllocateCommandBuffer ( Bool bIsPrimary )
{
    VkCommandBufferAllocateInfo AllocInfo{};
    AllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocInfo.commandPool        = Handle;
    AllocInfo.level              = bIsPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    AllocInfo.commandBufferCount = 1;

    VkCommandBuffer CmdBufferHandle = VK_NULL_HANDLE;
    VULKAN_CHECK( vkAllocateCommandBuffers( Device.GetLogicalDevice(), &AllocInfo, &CmdBufferHandle ) );

    AllocatedBuffers.PushBack( CmdBufferHandle );

    return MakeUnique<FVulkanCommandBuffer>( *this, CmdBufferHandle );
}

void LumenEngine::RHI::FVulkanCommandPool::Reset ()
{
    // Resets all allocated command buffers simultaneously (extremely fast driver path)
    VULKAN_CHECK( vkResetCommandPool( Device.GetLogicalDevice(), Handle, 0 ) );
}
