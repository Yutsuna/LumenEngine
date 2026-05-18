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

struct FSwapChainSupportDetails final
{
    VkSurfaceCapabilitiesKHR Capabilities = {};
    LumenEngine::TVector<VkSurfaceFormatKHR> Formats;
    LumenEngine::TVector<VkPresentModeKHR> PresentModes;
};

struct FSubmitInfoBundle final

{
    VkCommandBufferSubmitInfo CommandBufferSubmitInfo = {};
    VkSemaphoreSubmitInfo WaitSemaphoreSubmitInfo     = {};
    VkSemaphoreSubmitInfo SignalSemaphoreSubmitInfo   = {};
    VkSubmitInfo2 SubmitInfo                          = {};
};

VkSurfaceFormatKHR ChooseSwapSurfaceFormat ( const LumenEngine::TVector<VkSurfaceFormatKHR> &AvailableFormats, VkFormat DesiredFormat )
{
    for ( const VkSurfaceFormatKHR &Format : AvailableFormats )
    {
        if ( Format.format == DesiredFormat and Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            return Format;
        }
    }
    return AvailableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode ( const LumenEngine::TVector<VkPresentModeKHR> &AvailablePresentModes, Bool bVSync )
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

    VkExtent2D ActualExtent = { .width = Size.Width, .height = Size.Height };
    ActualExtent.width      = std::clamp( ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width );
    ActualExtent.height     = std::clamp( ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height );
    return ActualExtent;
}

[[nodiscard]] FSwapChainSupportDetails QuerySwapChainSupport ( VkPhysicalDevice InPhysicalDevice, VkSurfaceKHR InSurface )
{
    FSwapChainSupportDetails Details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( InPhysicalDevice, InSurface, &Details.Capabilities );

    LumenEngine::UInt32 FormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( InPhysicalDevice, InSurface, &FormatCount, nullptr );
    Details.Formats.resize( FormatCount );
    if ( FormatCount != 0 )
    {
        vkGetPhysicalDeviceSurfaceFormatsKHR( InPhysicalDevice, InSurface, &FormatCount, Details.Formats.data() );
    }

    LumenEngine::UInt32 PresentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR( InPhysicalDevice, InSurface, &PresentModeCount, nullptr );
    Details.PresentModes.resize( PresentModeCount );
    if ( PresentModeCount != 0 )
    {
        vkGetPhysicalDeviceSurfacePresentModesKHR( InPhysicalDevice, InSurface, &PresentModeCount, Details.PresentModes.data() );
    }

    return Details;
}

[[nodiscard]] VkSwapchainCreateInfoKHR CreateSwapChainCreateInfo ( VkSurfaceKHR InSurface,
                                                                   const VkSurfaceCapabilitiesKHR &InCapabilities,
                                                                   const VkSurfaceFormatKHR &InSurfaceFormat,
                                                                   VkPresentModeKHR InPresentMode,
                                                                   VkExtent2D InExtent,
                                                                   LumenEngine::UInt32 InImageCount,
                                                                   VkSwapchainKHR InOldSwapchainHandle )
{
    return VkSwapchainCreateInfoKHR{
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                 = nullptr,
        .flags                 = 0,
        .surface               = InSurface,
        .minImageCount         = InImageCount,
        .imageFormat           = InSurfaceFormat.format,
        .imageColorSpace       = InSurfaceFormat.colorSpace,
        .imageExtent           = InExtent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = InCapabilities.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = InPresentMode,
        .clipped               = VK_TRUE,
        .oldSwapchain          = InOldSwapchainHandle,
    };
}

[[nodiscard]] VkImageViewCreateInfo CreateImageViewCreateInfo ( VkImage InImage, VkFormat InImageFormat )
{
    return VkImageViewCreateInfo{
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .image    = InImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = InImageFormat,
        .components =
            VkComponentMapping{
                .r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = VkImageSubresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 },
    };
}

void BuildSubmitInfo2 ( FSubmitInfoBundle &OutSubmitBundle, VkCommandBuffer InCmd, VkSemaphore InWaitSemaphore, VkSemaphore InRenderSemaphore ) noexcept
{
    OutSubmitBundle.CommandBufferSubmitInfo = VkCommandBufferSubmitInfo{
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = InCmd,
        .deviceMask    = 0,
    };

    OutSubmitBundle.WaitSemaphoreSubmitInfo = VkSemaphoreSubmitInfo{
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = InWaitSemaphore,
        .value       = 0,
        .stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .deviceIndex = 0,
    };

    OutSubmitBundle.SignalSemaphoreSubmitInfo = VkSemaphoreSubmitInfo{
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = InRenderSemaphore,
        .value       = 0,
        .stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .deviceIndex = 0,
    };

    OutSubmitBundle.SubmitInfo = VkSubmitInfo2{
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0,
        .waitSemaphoreInfoCount   = 1,
        .pWaitSemaphoreInfos      = &OutSubmitBundle.WaitSemaphoreSubmitInfo,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &OutSubmitBundle.CommandBufferSubmitInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos    = &OutSubmitBundle.SignalSemaphoreSubmitInfo,
    };
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
    VkPhysicalDevice InPhysicalDevice, VkDevice InDevice, VkSurfaceKHR InSurface, VkFormat InSwapChainFormat, const Maths::FVec2u &InSize, Bool bInVSyncEnabled )
{
    const FSwapChainDescription Description{
        .Surface       = InSurface,
        .Format        = InSwapChainFormat,
        .Size          = InSize,
        .bVSyncEnabled = bInVSyncEnabled,
    };
    CreateInternal( InPhysicalDevice, InDevice, Description, VK_NULL_HANDLE );
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::Recreate (
    VkPhysicalDevice InPhysicalDevice, VkDevice InDevice, VkSurfaceKHR InSurface, VkFormat InSwapChainFormat, const Maths::FVec2u &InSize, Bool bInVSync )
{
    const FSwapChainDescription Description{
        .Surface       = InSurface,
        .Format        = InSwapChainFormat,
        .Size          = InSize,
        .bVSyncEnabled = bInVSync,
    };
    CreateInternal( InPhysicalDevice, InDevice, Description, SwapChainHandle );
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::CreateInternal ( VkPhysicalDevice InPhysicalDevice,
                                                                VkDevice InDevice,
                                                                const FSwapChainDescription &InDescription,
                                                                VkSwapchainKHR InOldSwapchainHandle )
{
    const FSwapChainSupportDetails Details = QuerySwapChainSupport( InPhysicalDevice, InDescription.Surface );
    const VkSurfaceFormatKHR SurfaceFormat = ChooseSwapSurfaceFormat( Details.Formats, InDescription.Format );
    const VkPresentModeKHR PresentMode     = ChooseSwapPresentMode( Details.PresentModes, InDescription.bVSyncEnabled );
    Extent                                 = ChooseSwapExtent( Details.Capabilities, InDescription.Size );
    ImageFormat                            = SurfaceFormat.format;

    UInt32 ImageCount = Details.Capabilities.minImageCount + 1;
    if ( Details.Capabilities.maxImageCount > 0 and ImageCount > Details.Capabilities.maxImageCount )
    {
        ImageCount = Details.Capabilities.maxImageCount;
    }

    const VkSwapchainCreateInfoKHR CreateInfo =
        CreateSwapChainCreateInfo( InDescription.Surface, Details.Capabilities, SurfaceFormat, PresentMode, Extent, ImageCount, InOldSwapchainHandle );

    VkSwapchainKHR NewSwapchainHandle = VK_NULL_HANDLE;
    LUMEN_VK_CHECK( vkCreateSwapchainKHR( InDevice, &CreateInfo, nullptr, &NewSwapchainHandle ) );

    if ( InOldSwapchainHandle != VK_NULL_HANDLE )
    {
        for ( VkImageView &View : ImageViews )
        {
            vkDestroyImageView( InDevice, View, nullptr );
        }
        for ( VkSemaphore &Semaphore : RenderSemaphores )
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
    for ( USize Index = 0; Index < Images.size(); ++Index )
    {
        const VkImageViewCreateInfo ViewInfo = CreateImageViewCreateInfo( Images[Index], ImageFormat );

        LUMEN_VK_CHECK( vkCreateImageView( InDevice, &ViewInfo, nullptr, &ImageViews[Index] ) );
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

    for ( const VkSemaphore &Semaphore : RenderSemaphores )
    {
        vkDestroySemaphore( InDevice, Semaphore, nullptr );
    }
    RenderSemaphores.clear();

    for ( const VkImageView &View : ImageViews )
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

VkFormat LumenEngine::VulkanRHI::FVulkanSwapChain::GetImageFormat () const noexcept
{
    return ImageFormat;
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

    if ( AcquireResult == VK_ERROR_OUT_OF_DATE_KHR )
    {
        bIsDirty = true;
        return { VK_NULL_HANDLE, 0 };
    }

    if ( AcquireResult == VK_SUBOPTIMAL_KHR )
    {
        bIsDirty = true;
    }
    else if ( AcquireResult != VK_SUCCESS )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to acquire swap chain image!" );
        return { VK_NULL_HANDLE, 0 };
    }

    return { Images[SwapChainImageIndex], SwapChainImageIndex };
}

void LumenEngine::VulkanRHI::FVulkanSwapChain::SubmitAndPresent ( VkCommandBuffer InCmd,
                                                                  VkQueue InGraphicsQueue,
                                                                  USize InFrameIndex,
                                                                  UInt32 InSwapChainImageIndex ) noexcept
{
    const FFrameData &Frame            = Frames[InFrameIndex];
    const VkSemaphore &RenderSemaphore = RenderSemaphores[InSwapChainImageIndex];
    const VkSemaphore &WaitSemaphore   = Frame.SwapChainSemaphore;
    FSubmitInfoBundle SubmitBundle;
    BuildSubmitInfo2( SubmitBundle, InCmd, WaitSemaphore, RenderSemaphore );

    LUMEN_VK_CHECK( vkQueueSubmit2( InGraphicsQueue, 1, &SubmitBundle.SubmitInfo, Frame.RenderFence ) );

    const VkPresentInfoKHR PresentInfo{
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext              = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &RenderSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &SwapChainHandle,
        .pImageIndices      = &InSwapChainImageIndex,
        .pResults           = nullptr,
    };

    const VkResult PresentResult = vkQueuePresentKHR( InGraphicsQueue, &PresentInfo );
    if ( PresentResult == VK_ERROR_OUT_OF_DATE_KHR or PresentResult == VK_SUBOPTIMAL_KHR )
    {
        bIsDirty = true;
        return;
    }

    if ( PresentResult != VK_SUCCESS )
    {
        LUMEN_LOG_ERROR( LogVulkanRHI, "Failed to present SwapChain image." );
        bIsDirty = true;
    }
}
