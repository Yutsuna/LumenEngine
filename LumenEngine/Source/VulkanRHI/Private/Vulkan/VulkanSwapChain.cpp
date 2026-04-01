/**
 * @file VulkanSwapChain.cpp
 * @brief VulkanSwapChain implementation
 */

#include "CoreTypes.hpp"
#include "Maths/Vec.hpp"

#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanSwapChain.hpp"

#include <limits>

const LumenEngine::FLogCategory LumenEngine::VulkanRHI::LogVulkanRHI( "VulkanRHI" );

namespace
{

constexpr LumenEngine::UInt64 NoTimeout = std::numeric_limits<LumenEngine::UInt64>::max();

}

void LumenEngine::VulkanRHI::FVulkanSwapChain::InitializeSynStructures ( VkDevice InDevice )
{
    const VkFenceCreateInfo FenceCreateInfo         = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
    const VkSemaphoreCreateInfo SemaphoreCreateInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0 };

    for ( FFrameData &Frame : Frames )
    {
        LUMEN_VK_CHECK( vkCreateFence( InDevice, &FenceCreateInfo, nullptr, &Frame.RenderFence ) );
        LUMEN_VK_CHECK( vkCreateSemaphore( InDevice, &SemaphoreCreateInfo, nullptr, &Frame.SwapChainSemaphore ) );
        LUMEN_VK_CHECK( vkCreateSemaphore( InDevice, &SemaphoreCreateInfo, nullptr, &Frame.RenderSemaphore ) );
    }
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::Create ( const vkb::Device &InDevice, VkFormat InSwapChainFormat, const Maths::FVec2u &InSize, bool bInVSyncEnabled )
{
    vkb::SwapchainBuilder SwapChainBuilder{ InDevice };
    const VkSurfaceFormatKHR DesiredFormat    = { .format = InSwapChainFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    const VkPresentModeKHR DesiredPresentMode = bInVSyncEnabled ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    vkb::Result<vkb::Swapchain> BuildResult   = SwapChainBuilder.set_desired_format( DesiredFormat )
                                                    .add_image_usage_flags( VK_IMAGE_USAGE_TRANSFER_DST_BIT )
                                                    .set_desired_present_mode( DesiredPresentMode )
                                                    .set_desired_extent( InSize.Width, InSize.Height )
                                                    .build();

    if ( not BuildResult.has_value() )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to create SwapChain: {}", BuildResult.error().message() );
    }

    SwapChain  = BuildResult.value();
    Images     = SwapChain.get_images().value();
    ImageViews = SwapChain.get_image_views().value();
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::Recreate ( const vkb::Device &InDevice, VkFormat InSwapChainFormat, const Maths::FVec2u &InSize, bool bInVSync )
{
    vkb::SwapchainBuilder SwapChainBuilder{ InDevice };
    const VkSurfaceFormatKHR DesiredFormat    = { .format = InSwapChainFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    const VkPresentModeKHR DesiredPresentMode = bInVSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    vkb::Result<vkb::Swapchain> BuildResult   = SwapChainBuilder.set_old_swapchain( SwapChain )
                                                    .set_desired_format( DesiredFormat )
                                                    .add_image_usage_flags( VK_IMAGE_USAGE_TRANSFER_DST_BIT )
                                                    .set_desired_present_mode( DesiredPresentMode )
                                                    .set_desired_extent( InSize.Width, InSize.Height )
                                                    .build();

    if ( not BuildResult.has_value() )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to recreate SwapChain: {}", BuildResult.error().message() );
    }

    vkb::destroy_swapchain( SwapChain );

    for ( VkImageView View : ImageViews )
    {
        vkDestroyImageView( InDevice.device, View, nullptr );
    }

    SwapChain  = BuildResult.value();
    Images     = SwapChain.get_images().value();
    ImageViews = SwapChain.get_image_views().value();
    bIsDirty   = false;
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::Cleanup ( VkDevice InDevice ) noexcept
{
    for ( USize FrameIndex = 0; FrameIndex < MaxFramesInFlight; ++FrameIndex )
    {
        vkDestroyFence( InDevice, Frames[FrameIndex].RenderFence, nullptr );
        vkDestroySemaphore( InDevice, Frames[FrameIndex].SwapChainSemaphore, nullptr );
        vkDestroySemaphore( InDevice, Frames[FrameIndex].RenderSemaphore, nullptr );
    }

    for ( VkImageView View : ImageViews )
    {
        vkDestroyImageView( InDevice, View, nullptr );
    }
    ImageViews.clear();

    vkb::destroy_swapchain( SwapChain );
}

VkExtent2D LumenEngine::VulkanRHI::FVulkanSwapChain::GetExtent () const noexcept
{
    return SwapChain.extent;
}

const LumenEngine::TVector<VkImage> &LumenEngine::VulkanRHI::FVulkanSwapChain::GetImages () const noexcept
{
    return Images;
}

VkImageView LumenEngine::VulkanRHI::FVulkanSwapChain::GetImageView ( const USize InImageIndex ) const noexcept
{
    return ImageViews[InImageIndex];
}

LumenEngine::Bool LumenEngine::VulkanRHI::FVulkanSwapChain::NeedsRecreation () const noexcept
{
    return bIsDirty;
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::BeginFrame ( VkDevice InDevice, USize InFrameIndex ) const
{
    const FFrameData &Frame = Frames[InFrameIndex];

    LUMEN_VK_CHECK( vkWaitForFences( InDevice, 1, &Frame.RenderFence, VK_TRUE, NoTimeout ) );
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::ResetFences ( VkDevice InDevice, USize InFrameIndex ) const
{
    const FFrameData &Frame = Frames[InFrameIndex];

    LUMEN_VK_CHECK( vkResetFences( InDevice, 1, &Frame.RenderFence ) );
}

std::pair<VkImage, LumenEngine::UInt32> LumenEngine::VulkanRHI::FVulkanSwapChain::AcquireNextImage ( VkDevice InDevice, USize InFrameIndex )
{
    UInt32 SwapChainImageIndex = 0;
    const VkResult AcquireResult =
        vkAcquireNextImageKHR( InDevice, SwapChain.swapchain, NoTimeout, Frames[InFrameIndex].SwapChainSemaphore, VK_NULL_HANDLE, &SwapChainImageIndex );

    if ( AcquireResult == VK_ERROR_OUT_OF_DATE_KHR || AcquireResult == VK_SUBOPTIMAL_KHR )
    {
        bIsDirty = true;
    }
    else if ( AcquireResult != VK_SUCCESS )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to acquire swap chain image!" );
    }

    return { Images[SwapChainImageIndex], SwapChainImageIndex };
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::SubmitAndPresent ( VkCommandBuffer InCmd,
                                                                  VkQueue InGraphicsQueue,
                                                                  USize InFrameIndex,
                                                                  UInt32 InSwapChainImageIndex ) noexcept
{
    const FFrameData &Frame                                 = Frames[InFrameIndex];
    const VkCommandBufferSubmitInfo CommandBufferSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .pNext = nullptr, .commandBuffer = InCmd, .deviceMask = 0 };
    const VkSemaphoreSubmitInfo WaitSemaphoreSubmitInfo   = { .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                                              .pNext       = nullptr,
                                                              .semaphore   = Frame.SwapChainSemaphore,
                                                              .value       = 1,
                                                              .stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                                              .deviceIndex = 0 };
    const VkSemaphoreSubmitInfo SignalSemaphoreSubmitInfo = { .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                                              .pNext       = nullptr,
                                                              .semaphore   = Frame.RenderSemaphore,
                                                              .value       = 1,
                                                              .stageMask   = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                                              .deviceIndex = 0 };
    const VkSubmitInfo2 SubmitInfo                        = { .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                                                              .pNext                    = nullptr,
                                                              .flags                    = 0,
                                                              .waitSemaphoreInfoCount   = 1,
                                                              .pWaitSemaphoreInfos      = &WaitSemaphoreSubmitInfo,
                                                              .commandBufferInfoCount   = 1,
                                                              .pCommandBufferInfos      = &CommandBufferSubmitInfo,
                                                              .signalSemaphoreInfoCount = 1,
                                                              .pSignalSemaphoreInfos    = &SignalSemaphoreSubmitInfo };

    LUMEN_VK_CHECK( vkQueueSubmit2( InGraphicsQueue, 1, &SubmitInfo, Frame.RenderFence ) );

    const VkPresentInfoKHR PresentInfo = { .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                           .pNext              = nullptr,
                                           .waitSemaphoreCount = 1,
                                           .pWaitSemaphores    = &Frame.RenderSemaphore,
                                           .swapchainCount     = 1,
                                           .pSwapchains        = &SwapChain.swapchain,
                                           .pImageIndices      = &InSwapChainImageIndex,
                                           .pResults           = nullptr };
    const VkResult PresentResult       = vkQueuePresentKHR( InGraphicsQueue, &PresentInfo );

    if ( PresentResult != VK_SUCCESS )
    {
        if ( PresentResult != VK_SUBOPTIMAL_KHR )
        {
            LUMEN_LOG_ERROR( LogVulkanRHI, "Failed to present SwapChain image." );
        }
        bIsDirty = true;
    }
}
