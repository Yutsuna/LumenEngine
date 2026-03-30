#include "VulkanBarrierBatcher.hpp"

void LumenEngine::RHI::FVulkanBarrierBatcher::AddMemoryBarrier ( VkAccessFlags InSrcAccessMask, VkAccessFlags InDstAccessMask )
{
    VkMemoryBarrier Barrier{};
    Barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    Barrier.srcAccessMask = InSrcAccessMask;
    Barrier.dstAccessMask = InDstAccessMask;
    MemoryBarriers.PushBack( Barrier );
}

void LumenEngine::RHI::FVulkanBarrierBatcher::AddImageBarrier (
    VkImage InImage, VkAccessFlags InSrcAccessMask, VkAccessFlags InDstAccessMask, VkImageLayout InOldLayout, VkImageLayout InNewLayout, VkImageAspectFlags InAspectMask )
{
    VkImageMemoryBarrier Barrier{};
    Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.srcAccessMask                   = InSrcAccessMask;
    Barrier.dstAccessMask                   = InDstAccessMask;
    Barrier.oldLayout                       = InOldLayout;
    Barrier.newLayout                       = InNewLayout;
    Barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image                           = InImage;
    Barrier.subresourceRange.aspectMask     = InAspectMask;
    Barrier.subresourceRange.baseMipLevel   = 0;
    Barrier.subresourceRange.levelCount     = 1; // Simplification, handle full mip chain later
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount     = 1;
    ImageBarriers.PushBack( Barrier );
}

void LumenEngine::RHI::FVulkanBarrierBatcher::Flush ( VkCommandBuffer InCmdBuffer, VkPipelineStageFlags InSrcStage, VkPipelineStageFlags InDstStage )
{
    if ( MemoryBarriers.empty() && ImageBarriers.empty() && BufferBarriers.empty() )
    {
        return;
    }

    vkCmdPipelineBarrier( InCmdBuffer, InSrcStage, InDstStage,
                          0, // Dependency flags
                          static_cast<UInt32>( MemoryBarriers.size() ), MemoryBarriers.data(), static_cast<UInt32>( BufferBarriers.size() ), BufferBarriers.data(),
                          static_cast<UInt32>( ImageBarriers.size() ), ImageBarriers.data() );

    Clear();
}

void LumenEngine::RHI::FVulkanBarrierBatcher::Clear () noexcept
{
    MemoryBarriers.clear();
    ImageBarriers.clear();
    BufferBarriers.clear();
}
