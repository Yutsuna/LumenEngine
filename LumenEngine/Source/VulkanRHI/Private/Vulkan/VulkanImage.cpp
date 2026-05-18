/**
 * @file VulkanImage.cpp
 * @brief Vulkan image wrapper for resource management implementation
 */

#include "Vulkan/VulkanImage.hpp"

LumenEngine::Maths::FVec2i LumenEngine::VulkanRHI::FVulkanImage::GetSize2D () const noexcept
{
    const Int32 Width  = static_cast<Int32>( Extent.width );
    const Int32 Height = static_cast<Int32>( Extent.height );

    return Maths::FVec2i( Width, Height );
}

VkExtent2D LumenEngine::VulkanRHI::FVulkanImage::GetExtent2D () const noexcept
{
    return VkExtent2D{ Extent.width, Extent.height };
}

LumenEngine::UInt32 LumenEngine::VulkanRHI::FVulkanImage::GetBindlessID () const noexcept
{
    return BindlessID;
}

void LumenEngine::VulkanRHI::FVulkanImage::SetBindlessID ( const UInt32 InID ) noexcept
{
    BindlessID = InID;
}

LumenEngine::Bool LumenEngine::VulkanRHI::FVulkanImage::IsInitialized () const noexcept
{
    return BindlessID != NullBindlessID;
}

void LumenEngine::VulkanRHI::FVulkanImage::Cleanup ( VmaAllocator InAllocator, VkDevice InDevice ) noexcept
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
}
