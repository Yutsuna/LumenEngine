/**
 * @file VulkanCommandBuffer.cpp
 * @brief Vulkan command buffer wrapper for recording rendering commands
 */

#include "Vulkan/VulkanCommandBuffer.hpp"
#include "Vulkan/VulkanCore.hpp"

LumenEngine::VulkanRHI::FVulkanCommandBuffer::FVulkanCommandBuffer ( VkCommandBuffer InHandle ) noexcept : Handle( InHandle )
{
    //
}

void LumenEngine::VulkanRHI::FVulkanCommandBuffer::Begin ( VkCommandBufferUsageFlags Flags )
{
    VkCommandBufferBeginInfo BeginInfo{};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags = Flags;

    LUMEN_VK_CHECK( vkBeginCommandBuffer( Handle, &BeginInfo ) );
}

void LumenEngine::VulkanRHI::FVulkanCommandBuffer::End ()
{
    LUMEN_VK_CHECK( vkEndCommandBuffer( Handle ) );
}

void LumenEngine::VulkanRHI::FVulkanCommandBuffer::PipelineBarrier ( const VkDependencyInfo &DepInfo ) noexcept
{
    vkCmdPipelineBarrier2( Handle, &DepInfo );
}

void LumenEngine::VulkanRHI::FVulkanCommandBuffer::TransitionImageLayout ( VkImage Image,
                                                                           VkImageLayout OldLayout,
                                                                           VkImageLayout NewLayout,
                                                                           VkImageAspectFlags AspectMask )
{
    VkImageMemoryBarrier2 Barrier{};
    Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    Barrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    Barrier.srcAccessMask                   = VK_ACCESS_2_MEMORY_WRITE_BIT;
    Barrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    Barrier.dstAccessMask                   = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    Barrier.oldLayout                       = OldLayout;
    Barrier.newLayout                       = NewLayout;
    Barrier.image                           = Image;
    Barrier.subresourceRange.aspectMask     = AspectMask;
    Barrier.subresourceRange.baseMipLevel   = 0;
    Barrier.subresourceRange.levelCount     = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount     = 1;

    VkDependencyInfo DepInfo{};
    DepInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    DepInfo.imageMemoryBarrierCount = 1;
    DepInfo.pImageMemoryBarriers    = &Barrier;

    PipelineBarrier( DepInfo );
}

VkCommandBuffer LumenEngine::VulkanRHI::FVulkanCommandBuffer::GetHandle () const noexcept
{
    return Handle;
}
