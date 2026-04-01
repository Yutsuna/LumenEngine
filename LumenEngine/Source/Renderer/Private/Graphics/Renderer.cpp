/**
 * @file Renderer.hpp
 * @brief Main Renderer class that manages the rendering pipeline and resources.
 */

#include "Graphics/Renderer.hpp"

#include "HAL/PlatformTime.hpp"
#include "Logging/LoggingCategory.hpp"
#include <cmath>
#include <cstdlib>
#include <vulkan/vulkan_core.h>

namespace
{

const LumenEngine::FLogCategory LogRenderer( "Renderer" );

}

LumenEngine::TUniquePtr<LumenEngine::FRenderer> LumenEngine::GRenderer = nullptr;

LumenEngine::FRenderer::~FRenderer () noexcept
{
    Shutdown();
}

void LumenEngine::FRenderer::Initialize ( const TSharedRef<FGenericWindow> &InWindow )
{
    RHI = MakeUnique<VulkanRHI::FVulkanRHI>();
    RHI->Initialize( InWindow );
    CreateCommandBuffers();

    LUMEN_LOG_INFO( LogRenderer, "Renderer initialized successfully." );
}

void LumenEngine::FRenderer::Shutdown () noexcept
{
    if ( not RHI )
    {
        LUMEN_LOG_WARNING( LogRenderer, "Renderer shutdown called without initialization." );
        return;
    }

    vkDeviceWaitIdle( RHI->GetDevice() );

    for ( UInt32 Index = 0; Index < VulkanRHI::MaxFramesInFlight; ++Index )
    {
        if ( CommandPools[Index].Handle != VK_NULL_HANDLE )
        {
            vkDestroyCommandPool( RHI->GetDevice(), CommandPools[Index].Handle, nullptr );
        }
    }

    RHI->Shutdown();
    RHI.Reset();
}

void LumenEngine::FRenderer::CreateCommandBuffers ()
{
    VkCommandPoolCreateInfo PoolInfo{};
    PoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    PoolInfo.queueFamilyIndex = RHI->GetGraphicsQueueFamily();

    VkCommandBufferAllocateInfo AllocInfo{};
    AllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocInfo.commandBufferCount = 1;

    for ( UInt32 Index = 0; Index < VulkanRHI::MaxFramesInFlight; ++Index )
    {
        LUMEN_VK_CHECK( vkCreateCommandPool( RHI->GetDevice(), &PoolInfo, nullptr, &CommandPools[Index].Handle ) );

        AllocInfo.commandPool = CommandPools[Index].Handle;
        LUMEN_VK_CHECK( vkAllocateCommandBuffers( RHI->GetDevice(), &AllocInfo, &CommandBuffers[Index].Handle ) );
    }
}

void LumenEngine::FRenderer::RenderFrame ()
{
    if ( not RHI )
    {
        LUMEN_LOG_WARNING( LogRenderer, "RenderFrame called without initialization." );
        return;
    }

    VulkanRHI::FVulkanSwapChain &Swapchain = RHI->GetSwapChain();
    VkDevice Device                        = RHI->GetDevice();
    const UInt32 CurrentFrame              = FrameIndex % VulkanRHI::MaxFramesInFlight;

    Swapchain.BeginFrame( Device, CurrentFrame );
    const auto &[Image, ImageIndex] = Swapchain.AcquireNextImage( Device, CurrentFrame );

    if ( Image == VK_NULL_HANDLE )
    {
        return;
    }

    Swapchain.ResetFences( Device, CurrentFrame );

    VkCommandBuffer CommandBuffer = CommandBuffers[CurrentFrame].Handle;
    LUMEN_VK_CHECK( vkResetCommandBuffer( CommandBuffer, 0 ) );

    VkCommandBufferBeginInfo BeginInfo{};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    LUMEN_VK_CHECK( vkBeginCommandBuffer( CommandBuffer, &BeginInfo ) );

    VkImageMemoryBarrier2 TransitionToGeneral{};
    TransitionToGeneral.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    TransitionToGeneral.srcStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    TransitionToGeneral.srcAccessMask                   = 0;
    TransitionToGeneral.dstStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    TransitionToGeneral.dstAccessMask                   = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    TransitionToGeneral.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    TransitionToGeneral.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    TransitionToGeneral.image                           = Image;
    TransitionToGeneral.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    TransitionToGeneral.subresourceRange.baseMipLevel   = 0;
    TransitionToGeneral.subresourceRange.levelCount     = 1;
    TransitionToGeneral.subresourceRange.baseArrayLayer = 0;
    TransitionToGeneral.subresourceRange.layerCount     = 1;

    VkDependencyInfo DepInfo1{};
    DepInfo1.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    DepInfo1.imageMemoryBarrierCount = 1;
    DepInfo1.pImageMemoryBarriers    = &TransitionToGeneral;

    vkCmdPipelineBarrier2( CommandBuffer, &DepInfo1 );

    const Float32 Time = static_cast<Float32>( HAL::FPlatformTime::Seconds() );

    VkClearColorValue ClearColor{};
    ClearColor.float32[0]                    = std::abs( std::sin( Time * 0.5F ) );
    ClearColor.float32[1]                    = 0.2F;
    ClearColor.float32[2]                    = std::abs( std::cos( Time * 0.3F ) );
    ClearColor.float32[3]                    = 1.0F;
    const VkImageSubresourceRange ClearRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdClearColorImage( CommandBuffer, Image, VK_IMAGE_LAYOUT_GENERAL, &ClearColor, 1, &ClearRange );

    VkImageMemoryBarrier2 TransitionToPresent{};
    TransitionToPresent.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    TransitionToPresent.srcStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    TransitionToPresent.srcAccessMask                   = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    TransitionToPresent.dstStageMask                    = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    TransitionToPresent.dstAccessMask                   = 0;
    TransitionToPresent.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    TransitionToPresent.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    TransitionToPresent.image                           = Image;
    TransitionToPresent.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    TransitionToPresent.subresourceRange.baseMipLevel   = 0;
    TransitionToPresent.subresourceRange.levelCount     = 1;
    TransitionToPresent.subresourceRange.baseArrayLayer = 0;
    TransitionToPresent.subresourceRange.layerCount     = 1;

    VkDependencyInfo DepInfo2{};
    DepInfo2.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    DepInfo2.imageMemoryBarrierCount = 1;
    DepInfo2.pImageMemoryBarriers    = &TransitionToPresent;
    vkCmdPipelineBarrier2( CommandBuffer, &DepInfo2 );

    LUMEN_VK_CHECK( vkEndCommandBuffer( CommandBuffer ) );

    Swapchain.SubmitAndPresent( CommandBuffer, RHI->GetGraphicsQueue(), CurrentFrame, ImageIndex );
    ++FrameIndex;
}
