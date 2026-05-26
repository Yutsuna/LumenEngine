/**
 * @file VulkanRenderContextManager.cpp
 * @brief Implementation of the VulkanRenderContextManager utility namespace.
 */

#include "Vulkan/VulkanRenderContextManager.hpp"

/**
 * Private
 */

namespace
{

void TransitionImageLayoutInternal (
    VkCommandBuffer InCmd, VkImage InImage, VkImageLayout InOldLayout, VkImageLayout InNewLayout, VkImageAspectFlags InAspectMask ) noexcept
{
    VkImageMemoryBarrier2 Barrier{};
    Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    Barrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    Barrier.srcAccessMask                   = VK_ACCESS_2_MEMORY_WRITE_BIT;
    Barrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    Barrier.dstAccessMask                   = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    Barrier.oldLayout                       = InOldLayout;
    Barrier.newLayout                       = InNewLayout;
    Barrier.image                           = InImage;
    Barrier.subresourceRange.aspectMask     = InAspectMask;
    Barrier.subresourceRange.baseMipLevel   = 0;
    Barrier.subresourceRange.levelCount     = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount     = 1;

    VkDependencyInfo DepInfo{};
    DepInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    DepInfo.imageMemoryBarrierCount = 1;
    DepInfo.pImageMemoryBarriers    = &Barrier;

    vkCmdPipelineBarrier2( InCmd, &DepInfo );
}

} // namespace

/**
 * Public
 */

void LumenEngine::VulkanRHI::VulkanRenderContextManager::BeginRendering ( VkCommandBuffer InCmd,
                                                                          VkImage InPresentImage,
                                                                          VkImageView InPresentView,
                                                                          VkFormat /*InFormat*/,
                                                                          VkExtent2D InExtent,
                                                                          const FVulkanMsaaManager &InMsaaManager,
                                                                          const Float32 InClearColor[4] ) noexcept
{
    const VkSampleCountFlagBits ActiveMsaaSamples = InMsaaManager.GetActiveSamples();
    const FVulkanMsaaRenderTarget &MsaaTarget     = InMsaaManager.GetRenderTarget();

    VkRenderingAttachmentInfo ColorAttachment{};
    ColorAttachment.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    ColorAttachment.clearValue.color = { { InClearColor[0], InClearColor[1], InClearColor[2], InClearColor[3] } };

    if ( ActiveMsaaSamples > VK_SAMPLE_COUNT_1_BIT )
    {
        TransitionImageLayoutInternal( InCmd, MsaaTarget.GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT );

        TransitionImageLayoutInternal( InCmd, InPresentImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT );

        ColorAttachment.imageView          = MsaaTarget.GetImageView();
        ColorAttachment.imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachment.loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR;
        ColorAttachment.storeOp            = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT;
        ColorAttachment.resolveImageView   = InPresentView;
        ColorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else
    {
        TransitionImageLayoutInternal( InCmd, InPresentImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT );

        ColorAttachment.imageView   = InPresentView;
        ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        ColorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    }

    VkRenderingInfo RenderInfo{};
    RenderInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    RenderInfo.renderArea.extent    = InExtent;
    RenderInfo.layerCount           = 1;
    RenderInfo.colorAttachmentCount = 1;
    RenderInfo.pColorAttachments    = &ColorAttachment;

    vkCmdBeginRendering( InCmd, &RenderInfo );

    const VkViewport Viewport{
        .x        = 0.F,
        .y        = 0.F,
        .width    = static_cast<Float32>( InExtent.width ),
        .height   = static_cast<Float32>( InExtent.height ),
        .minDepth = 0.F,
        .maxDepth = 1.F,
    };
    const VkRect2D Scissor{
        .offset = VkOffset2D{ .x = 0, .y = 0 },
        .extent = InExtent,
    };

    vkCmdSetViewport( InCmd, 0, 1, &Viewport );
    vkCmdSetScissor( InCmd, 0, 1, &Scissor );
}

void LumenEngine::VulkanRHI::VulkanRenderContextManager::TransitionPresentImageToPresentSource ( VkCommandBuffer InCmd, VkImage InPresentImage ) noexcept
{
    TransitionImageLayoutInternal( InCmd, InPresentImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT );
}
