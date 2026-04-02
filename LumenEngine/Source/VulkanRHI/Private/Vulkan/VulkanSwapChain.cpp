/**
 * @file VulkanSwapChain.cpp
 * @brief VulkanSwapChain implementation
 */

#include "CoreTypes.hpp"
#include "Maths/Vec.hpp"

#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanSwapChain.hpp"

#include <algorithm>
#include <limits>

const LumenEngine::FLogCategory LumenEngine::VulkanRHI::LogVulkanRHI( "VulkanRHI" );

namespace
{

constexpr LumenEngine::UInt64 NoTimeout = std::numeric_limits<LumenEngine::UInt64>::max();

VkSurfaceFormatKHR ChooseSwapSurfaceFormat ( const LumenEngine::TVector<VkSurfaceFormatKHR> &AvailableFormats, VkFormat DesiredFormat )
{
    for ( const VkSurfaceFormatKHR &Format : AvailableFormats )
    {
        if ( Format.format == DesiredFormat && Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            return Format;
        }
    }
    return AvailableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode ( const LumenEngine::TVector<VkPresentModeKHR> &AvailablePresentModes, bool bVSync )
{
    if ( bVSync )
    {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    for ( const VkPresentModeKHR &Mode : AvailablePresentModes )
    {
        if ( Mode == VK_PRESENT_MODE_MAILBOX_KHR )
        {
            return Mode;
        }
    }

    return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

VkExtent2D ChooseSwapExtent ( const VkSurfaceCapabilitiesKHR &Capabilities, const LumenEngine::Maths::FVec2u &Size )
{
    if ( Capabilities.currentExtent.width != std::numeric_limits<LumenEngine::UInt32>::max() )
    {
        return Capabilities.currentExtent;
    }

    VkExtent2D ActualExtent = { Size.Width, Size.Height };
    ActualExtent.width      = std::clamp( ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width );
    ActualExtent.height     = std::clamp( ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height );
    return ActualExtent;
}

} // namespace

void LumenEngine::VulkanRHI::FVulkanSwapChain::InitializeSynStructures ( VkDevice InDevice )
{
    const VkFenceCreateInfo FenceCreateInfo         = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
    const VkSemaphoreCreateInfo SemaphoreCreateInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0 };

    for ( FFrameData &Frame : Frames )
    {
        LUMEN_VK_CHECK( vkCreateFence( InDevice, &FenceCreateInfo, nullptr, &Frame.RenderFence ) );
        LUMEN_VK_CHECK( vkCreateSemaphore( InDevice, &SemaphoreCreateInfo, nullptr, &Frame.SwapChainSemaphore ) );
    }
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::Create (
    VkPhysicalDevice InPhysicalDevice, VkDevice InDevice, VkSurfaceKHR InSurface, VkFormat InSwapChainFormat, const Maths::FVec2u &InSize, bool bInVSyncEnabled )
{
    CreateInternal( InPhysicalDevice, InDevice, InSurface, InSwapChainFormat, InSize, bInVSyncEnabled, VK_NULL_HANDLE );
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::Recreate (
    VkPhysicalDevice InPhysicalDevice, VkDevice InDevice, VkSurfaceKHR InSurface, VkFormat InSwapChainFormat, const Maths::FVec2u &InSize, bool bInVSync )
{
    CreateInternal( InPhysicalDevice, InDevice, InSurface, InSwapChainFormat, InSize, bInVSync, SwapChainHandle );
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::CreateInternal ( VkPhysicalDevice InPhysicalDevice,
                                                                VkDevice InDevice,
                                                                VkSurfaceKHR InSurface,
                                                                VkFormat InSwapChainFormat,
                                                                const Maths::FVec2u &InSize,
                                                                bool bInVSyncEnabled,
                                                                VkSwapchainKHR InOldSwapchainHandle )
{
    VkSurfaceCapabilitiesKHR Capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( InPhysicalDevice, InSurface, &Capabilities );

    UInt32 FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR( InPhysicalDevice, InSurface, &FormatCount, nullptr );
    TVector<VkSurfaceFormatKHR> Formats( FormatCount );
    if ( FormatCount != 0 )
    {
        vkGetPhysicalDeviceSurfaceFormatsKHR( InPhysicalDevice, InSurface, &FormatCount, Formats.data() );
    }

    UInt32 PresentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR( InPhysicalDevice, InSurface, &PresentModeCount, nullptr );
    TVector<VkPresentModeKHR> PresentModes( PresentModeCount );
    if ( PresentModeCount != 0 )
    {
        vkGetPhysicalDeviceSurfacePresentModesKHR( InPhysicalDevice, InSurface, &PresentModeCount, PresentModes.data() );
    }

    const VkSurfaceFormatKHR SurfaceFormat = ChooseSwapSurfaceFormat( Formats, InSwapChainFormat );
    const VkPresentModeKHR PresentMode     = ChooseSwapPresentMode( PresentModes, bInVSyncEnabled );
    Extent                                 = ChooseSwapExtent( Capabilities, InSize );
    ImageFormat                            = SurfaceFormat.format;

    UInt32 ImageCount = Capabilities.minImageCount + 1;
    if ( Capabilities.maxImageCount > 0 && ImageCount > Capabilities.maxImageCount )
    {
        ImageCount = Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR CreateInfo{};
    CreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    CreateInfo.surface          = InSurface;
    CreateInfo.minImageCount    = ImageCount;
    CreateInfo.imageFormat      = SurfaceFormat.format;
    CreateInfo.imageColorSpace  = SurfaceFormat.colorSpace;
    CreateInfo.imageExtent      = Extent;
    CreateInfo.imageArrayLayers = 1;
    CreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    CreateInfo.preTransform     = Capabilities.currentTransform;
    CreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    CreateInfo.presentMode      = PresentMode;
    CreateInfo.clipped          = VK_TRUE;
    CreateInfo.oldSwapchain     = InOldSwapchainHandle;

    VkSwapchainKHR NewSwapchainHandle;
    LUMEN_VK_CHECK( vkCreateSwapchainKHR( InDevice, &CreateInfo, nullptr, &NewSwapchainHandle ) );

    if ( InOldSwapchainHandle != VK_NULL_HANDLE )
    {
        for ( VkImageView View : ImageViews )
        {
            vkDestroyImageView( InDevice, View, nullptr );
        }
        for ( VkSemaphore Semaphore : RenderSemaphores )
        {
            vkDestroySemaphore( InDevice, Semaphore, nullptr );
        }
        vkDestroySwapchainKHR( InDevice, InOldSwapchainHandle, nullptr );
    }

    SwapChainHandle = NewSwapchainHandle;

    vkGetSwapchainImagesKHR( InDevice, SwapChainHandle, &ImageCount, nullptr );
    Images.resize( ImageCount );
    vkGetSwapchainImagesKHR( InDevice, SwapChainHandle, &ImageCount, Images.data() );

    ImageViews.resize( Images.size() );
    for ( USize i = 0; i < Images.size(); ++i )
    {
        VkImageViewCreateInfo ViewInfo{};
        ViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ViewInfo.image                           = Images[i];
        ViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        ViewInfo.format                          = ImageFormat;
        ViewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ViewInfo.subresourceRange.baseMipLevel   = 0;
        ViewInfo.subresourceRange.levelCount     = 1;
        ViewInfo.subresourceRange.baseArrayLayer = 0;
        ViewInfo.subresourceRange.layerCount     = 1;

        LUMEN_VK_CHECK( vkCreateImageView( InDevice, &ViewInfo, nullptr, &ImageViews[i] ) );
    }

    const VkSemaphoreCreateInfo SemaphoreCreateInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0 };
    RenderSemaphores.resize( Images.size() );
    for ( VkSemaphore &Semaphore : RenderSemaphores )
    {
        LUMEN_VK_CHECK( vkCreateSemaphore( InDevice, &SemaphoreCreateInfo, nullptr, &Semaphore ) );
    }

    bIsDirty = false;
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::Cleanup ( VkDevice InDevice ) noexcept
{
    for ( USize FrameIndex = 0; FrameIndex < MaxFramesInFlight; ++FrameIndex )
    {
        vkDestroyFence( InDevice, Frames[FrameIndex].RenderFence, nullptr );
        vkDestroySemaphore( InDevice, Frames[FrameIndex].SwapChainSemaphore, nullptr );
    }

    for ( VkSemaphore Semaphore : RenderSemaphores )
    {
        vkDestroySemaphore( InDevice, Semaphore, nullptr );
    }
    RenderSemaphores.clear();

    for ( VkImageView View : ImageViews )
    {
        vkDestroyImageView( InDevice, View, nullptr );
    }
    ImageViews.clear();

    if ( SwapChainHandle != VK_NULL_HANDLE )
    {
        vkDestroySwapchainKHR( InDevice, SwapChainHandle, nullptr );
        SwapChainHandle = VK_NULL_HANDLE;
    }
}

VkExtent2D LumenEngine::VulkanRHI::FVulkanSwapChain::GetExtent () const noexcept
{
    return Extent;
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
        vkAcquireNextImageKHR( InDevice, SwapChainHandle, NoTimeout, Frames[InFrameIndex].SwapChainSemaphore, VK_NULL_HANDLE, &SwapChainImageIndex );

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
    const VkSemaphore RenderSemaphore                       = RenderSemaphores[InSwapChainImageIndex];
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
                                                              .semaphore   = RenderSemaphore,
                                                              .value       = 1,
                                                              .stageMask   = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                                              .deviceIndex = 0 };

    const VkSubmitInfo2 SubmitInfo = { .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
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
                                           .pWaitSemaphores    = &RenderSemaphore,
                                           .swapchainCount     = 1,
                                           .pSwapchains        = &SwapChainHandle,
                                           .pImageIndices      = &InSwapChainImageIndex,
                                           .pResults           = nullptr };

    const VkResult PresentResult = vkQueuePresentKHR( InGraphicsQueue, &PresentInfo );

    if ( PresentResult != VK_SUCCESS )
    {
        if ( PresentResult != VK_SUBOPTIMAL_KHR )
        {
            LUMEN_LOG_ERROR( LogVulkanRHI, "Failed to present SwapChain image." );
        }
        bIsDirty = true;
    }
}
