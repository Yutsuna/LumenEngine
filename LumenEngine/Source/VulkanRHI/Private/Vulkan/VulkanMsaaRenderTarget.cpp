/**
 * @file VulkanMsaaRenderTarget.cpp
 * @brief Implementation of the FVulkanMsaaRenderTarget class for transient MSAA targets.
 */

#include "Vulkan/VulkanMsaaRenderTarget.hpp"
#include "Vulkan/VulkanCore.hpp"

void LumenEngine::VulkanRHI::FVulkanMsaaRenderTarget::Create (
    VmaAllocator InAllocator, VkDevice InDevice, VkFormat InFormat, VkExtent2D InExtent, VkSampleCountFlagBits InSamples )
{
    Destroy( InAllocator, InDevice );

    if ( InSamples == VK_SAMPLE_COUNT_1_BIT )
    {
        return;
    }

    Samples = InSamples;
    Format  = InFormat;
    Extent  = InExtent;

    VkImageCreateInfo ImageCI{ .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                               .pNext                 = nullptr,
                               .flags                 = 0,
                               .imageType             = VK_IMAGE_TYPE_2D,
                               .format                = Format,
                               .extent                = { .width = Extent.width, .height = Extent.height, .depth = 1U },
                               .mipLevels             = 1,
                               .arrayLayers           = 1,
                               .samples               = Samples,
                               .tiling                = VK_IMAGE_TILING_OPTIMAL,
                               .usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                               .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                               .queueFamilyIndexCount = 0,
                               .pQueueFamilyIndices   = nullptr,
                               .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED };

    VmaAllocationCreateInfo AllocCI{ .flags          = 0,
                                     .usage          = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                                     .requiredFlags  = 0,
                                     .preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
                                     .memoryTypeBits = 0,
                                     .pool           = VK_NULL_HANDLE,
                                     .pUserData      = nullptr,
                                     .priority       = 0.0F };

    LUMEN_VK_CHECK( vmaCreateImage( InAllocator, &ImageCI, &AllocCI, &Image, &Allocation, nullptr ) );

    VkImageViewCreateInfo ViewCI{
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext      = nullptr,
        .flags      = 0,
        .image      = Image,
        .viewType   = VK_IMAGE_VIEW_TYPE_2D,
        .format     = Format,
        .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 } };

    LUMEN_VK_CHECK( vkCreateImageView( InDevice, &ViewCI, nullptr, &ImageView ) );
}

void LumenEngine::VulkanRHI::FVulkanMsaaRenderTarget::Destroy ( VmaAllocator InAllocator, VkDevice InDevice ) noexcept
{
    if ( ImageView != VK_NULL_HANDLE )
    {
        vkDestroyImageView( InDevice, ImageView, nullptr );
        ImageView = VK_NULL_HANDLE;
    }
    if ( Image != VK_NULL_HANDLE )
    {
        vmaDestroyImage( InAllocator, Image, Allocation );
        Image      = VK_NULL_HANDLE;
        Allocation = VK_NULL_HANDLE;
    }
    Samples = VK_SAMPLE_COUNT_1_BIT;
    Format  = VK_FORMAT_UNDEFINED;
    Extent  = {};
}

void LumenEngine::VulkanRHI::FVulkanMsaaRenderTarget::RecreateIfNeeded (
    VmaAllocator InAllocator, VkDevice InDevice, VkFormat InFormat, VkExtent2D InExtent, VkSampleCountFlagBits InSamples )
{
    if ( Samples != InSamples or Extent.width != InExtent.width or Extent.height != InExtent.height or Format != InFormat )
    {
        Create( InAllocator, InDevice, InFormat, InExtent, InSamples );
    }
}

void LumenEngine::VulkanRHI::FVulkanMsaaRenderTarget::ReleaseOwnership ( VkImage &OutImage, VkImageView &OutView, VmaAllocation &OutAllocation ) noexcept
{
    OutImage      = Image;
    OutView       = ImageView;
    OutAllocation = Allocation;

    Image      = VK_NULL_HANDLE;
    ImageView  = VK_NULL_HANDLE;
    Allocation = VK_NULL_HANDLE;
    Samples    = VK_SAMPLE_COUNT_1_BIT;
    Format     = VK_FORMAT_UNDEFINED;
    Extent     = {};
}

VkImage LumenEngine::VulkanRHI::FVulkanMsaaRenderTarget::GetImage () const noexcept
{
    return Image;
}

VkImageView LumenEngine::VulkanRHI::FVulkanMsaaRenderTarget::GetImageView () const noexcept
{
    return ImageView;
}

VkSampleCountFlagBits LumenEngine::VulkanRHI::FVulkanMsaaRenderTarget::GetSamples () const noexcept
{
    return Samples;
}
