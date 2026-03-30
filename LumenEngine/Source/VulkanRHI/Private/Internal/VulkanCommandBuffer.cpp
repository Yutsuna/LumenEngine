#include "VulkanCommandBuffer.hpp"
#include "Logging/Logger.hpp"
#include "VulkanCommandPool.hpp"
#include "VulkanCore.hpp"

LumenEngine::RHI::FVulkanCommandBuffer::FVulkanCommandBuffer ( FVulkanCommandPool &InPool, VkCommandBuffer InHandle ) noexcept : ParentPool( InPool ), Handle( InHandle )
{
}

void LumenEngine::RHI::FVulkanCommandBuffer::Begin ( VkCommandBufferUsageFlags InUsageFlags )
{
    if ( bIsRecording )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Attempted to Begin() a command buffer that is already recording!" );
    }

    VkCommandBufferBeginInfo BeginInfo{};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags = InUsageFlags;

    VULKAN_CHECK( vkBeginCommandBuffer( Handle, &BeginInfo ) );
    bIsRecording = true;
}

void LumenEngine::RHI::FVulkanCommandBuffer::End ()
{
    if ( !bIsRecording )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Attempted to End() a command buffer that is not recording!" );
    }

    // Always flush any pending barriers before closing the command buffer
    BarrierBatcher.Flush( Handle, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT );

    VULKAN_CHECK( vkEndCommandBuffer( Handle ) );
    bIsRecording = false;
}
