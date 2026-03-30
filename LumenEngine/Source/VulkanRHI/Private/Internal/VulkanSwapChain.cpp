/**
 * @file VulkanSwapChain.cpp
 * @brief Implementation of the Vulkan SwapChain wrapper class
 */

#include "VulkanSwapChain.hpp"
#include "VulkanCore.hpp"
#include "VulkanDevice.hpp"
#include "VulkanQueue.hpp"

#include "Logging/Logger.hpp"

LumenEngine::RHI::FVulkanSwapChain::FVulkanSwapChain ( VkInstance InInstance,
                                                       FVulkanDevice &InDevice,
                                                       VkSurfaceKHR InSurface,
                                                       VkSwapchainKHR InSwapChain,
                                                       const Maths::FVec2u &InSize,
                                                       const UInt32 NumSwapChainImages ) noexcept
    : Instance( InInstance ), Device( InDevice ), Surface( InSurface ), Handle( InSwapChain )
{
    SwapChainValidation();

    // Retrieve swapchain images
    UInt32 ImageCount = 0;
    VULKAN_CHECK( vkGetSwapchainImagesKHR( Device.GetLogicalDevice(), Handle, &ImageCount, nullptr ) );
    Images.Resize( ImageCount );
    VULKAN_CHECK( vkGetSwapchainImagesKHR( Device.GetLogicalDevice(), Handle, &ImageCount, Images.GetData() ) );

    // Create synchronization objects
    VkSemaphoreCreateInfo SemaphoreInfo{};
    SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo FenceInfo{};
    FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first frame doesn't wait

    for ( UInt32 Index = 0; Index < MAX_FRAMES_IN_FLIGHT; ++Index )
    {
        VULKAN_CHECK( vkCreateSemaphore( Device.GetLogicalDevice(), &SemaphoreInfo, nullptr, &FrameSync[Index].ImageAvailableSemaphore ) );
        VULKAN_CHECK( vkCreateSemaphore( Device.GetLogicalDevice(), &SemaphoreInfo, nullptr, &FrameSync[Index].RenderFinishedSemaphore ) );
        VULKAN_CHECK( vkCreateFence( Device.GetLogicalDevice(), &FenceInfo, nullptr, &FrameSync[Index].InFlightFence ) );
    }
}

LumenEngine::RHI::FVulkanSwapChain::~FVulkanSwapChain () noexcept
{
    Destroy();
}

LumenEngine::UInt32 LumenEngine::RHI::FVulkanSwapChain::AcquireNextImage ()
{
    // Wait for previous frame to finish using synchronization objects of the current "slot"
    VULKAN_CHECK( vkWaitForFences( Device.GetLogicalDevice(), 1, &FrameSync[CurrentFrame].InFlightFence, VK_TRUE, UINT64_MAX ) );

    VkResult Result =
        vkAcquireNextImageKHR( Device.GetLogicalDevice(), Handle, UINT64_MAX, FrameSync[CurrentFrame].ImageAvailableSemaphore, VK_NULL_HANDLE, &CurrentImageIndex );

    if ( Result == VK_ERROR_OUT_OF_DATE_KHR )
    {
        return InvalidSwapChainImageIndex;
    }
    else if ( Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR )
    {
        VULKAN_CHECK( Result );
    }

    // Only reset the fence if we are actually submitting work
    VULKAN_CHECK( vkResetFences( Device.GetLogicalDevice(), 1, &FrameSync[CurrentFrame].InFlightFence ) );

    return CurrentImageIndex;
}

LumenEngine::RHI::ESwapChainStatus LumenEngine::RHI::FVulkanSwapChain::Present ()
{
    VkPresentInfoKHR PresentInfo{};
    PresentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores    = &FrameSync[CurrentFrame].RenderFinishedSemaphore;

    VkSwapchainKHR Swapchains[] = { Handle };
    PresentInfo.swapchainCount  = 1;
    PresentInfo.pSwapchains     = Swapchains;
    PresentInfo.pImageIndices   = &CurrentImageIndex;

    VkResult Result = vkQueuePresentKHR( Device.GetPresentQueue().GetHandle(), &PresentInfo );

    // Advance frame index
    CurrentFrame = ( CurrentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;

    if ( Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR )
    {
        return ESwapChainStatus::OutOfDate;
    }
    else if ( Result == VK_ERROR_SURFACE_LOST_KHR )
    {
        return ESwapChainStatus::SurfaceLost;
    }
    else
    {
        VULKAN_CHECK( Result );
    }

    return ESwapChainStatus::Healthy;
}

void LumenEngine::RHI::FVulkanSwapChain::Destroy () noexcept
{
    if ( Device.GetLogicalDevice() != VK_NULL_HANDLE )
    {
        vkDeviceWaitIdle( Device.GetLogicalDevice() );

        for ( UInt32 Index = 0; Index < MAX_FRAMES_IN_FLIGHT; ++Index )
        {
            if ( FrameSync[Index].ImageAvailableSemaphore != VK_NULL_HANDLE )
                vkDestroySemaphore( Device.GetLogicalDevice(), FrameSync[Index].ImageAvailableSemaphore, nullptr );
            if ( FrameSync[Index].RenderFinishedSemaphore != VK_NULL_HANDLE )
                vkDestroySemaphore( Device.GetLogicalDevice(), FrameSync[Index].RenderFinishedSemaphore, nullptr );
            if ( FrameSync[Index].InFlightFence != VK_NULL_HANDLE )
                vkDestroyFence( Device.GetLogicalDevice(), FrameSync[Index].InFlightFence, nullptr );
        }

        if ( Handle != VK_NULL_HANDLE )
        {
            vkDestroySwapchainKHR( Device.GetLogicalDevice(), Handle, nullptr );
            Handle = VK_NULL_HANDLE;
        }
    }
}

void LumenEngine::RHI::FVulkanSwapChain::SwapChainValidation ()
{
    if ( Device.GetLogicalDevice() == VK_NULL_HANDLE )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Invalid Vulkan device provided to FVulkanSwapChain" );
    }
    if ( Surface == VK_NULL_HANDLE )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Invalid Vulkan surface provided to FVulkanSwapChain" );
    }
    if ( Handle == VK_NULL_HANDLE )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Invalid Vulkan swap chain provided to FVulkanSwapChain" );
    }
}
