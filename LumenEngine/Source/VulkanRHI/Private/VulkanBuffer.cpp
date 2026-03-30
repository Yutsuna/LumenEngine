/**
 * @file VulkanBuffer.cpp
 * @brief Implementation of the FVulkanBuffer class
 */

#include "VulkanBuffer.hpp"
#include "VulkanRHI.hpp"
#include "VulkanDevice.hpp"
#include "VulkanCore.hpp"

namespace LumenEngine
{
namespace RHI
{
namespace
{
    /**
     * @brief Helper to find the memory type index that satisfies the requirements and properties
     */
    static UInt32 FindMemoryType ( VkPhysicalDevice InPhysicalDevice, UInt32 InTypeFilter, VkMemoryPropertyFlags InProperties )
    {
        VkPhysicalDeviceMemoryProperties MemProperties;
        vkGetPhysicalDeviceMemoryProperties( InPhysicalDevice, &MemProperties );

        for ( UInt32 Index = 0; Index < MemProperties.memoryTypeCount; ++Index )
        {
            if ( ( InTypeFilter & ( 1 << Index ) ) && ( MemProperties.memoryTypes[Index].propertyFlags & InProperties ) == InProperties )
            {
                return Index;
            }
        }

        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to find suitable memory type for Vulkan buffer!" );
        return static_cast<UInt32>( -1 );
    }
} // namespace
} // namespace RHI
} // namespace LumenEngine

LumenEngine::RHI::FVulkanBuffer::FVulkanBuffer ( VkDeviceSize InSize, VkBufferUsageFlags InUsage, VkMemoryPropertyFlags InProperties ) noexcept
    : Size( InSize )
{
    FVulkanDevice &DeviceWrapper = FVulkanRHI::Get().GetDevice();

    VkBufferCreateInfo BufferInfo{};
    BufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size        = Size;
    BufferInfo.usage       = InUsage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VULKAN_CHECK( vkCreateBuffer( DeviceWrapper.GetLogicalDevice(), &BufferInfo, nullptr, &Buffer ) );

    VkMemoryRequirements MemRequirements;
    vkGetBufferMemoryRequirements( DeviceWrapper.GetLogicalDevice(), Buffer, &MemRequirements );

    VkMemoryAllocateInfo AllocInfo{};
    AllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize  = MemRequirements.size;
    AllocInfo.memoryTypeIndex = FindMemoryType( DeviceWrapper.GetPhysicalDevice(), MemRequirements.memoryTypeBits, InProperties );

    VULKAN_CHECK( vkAllocateMemory( DeviceWrapper.GetLogicalDevice(), &AllocInfo, nullptr, &Memory ) );

    VULKAN_CHECK( vkBindBufferMemory( DeviceWrapper.GetLogicalDevice(), Buffer, Memory, 0 ) );
}

LumenEngine::RHI::FVulkanBuffer::~FVulkanBuffer () noexcept
{
    if ( Buffer != VK_NULL_HANDLE || Memory != VK_NULL_HANDLE )
    {
        Release();
    }
}

void LumenEngine::RHI::FVulkanBuffer::Release ()
{
    FVulkanDevice &DeviceWrapper = FVulkanRHI::Get().GetDevice();

    if ( Buffer != VK_NULL_HANDLE )
    {
        vkDestroyBuffer( DeviceWrapper.GetLogicalDevice(), Buffer, nullptr );
        Buffer = VK_NULL_HANDLE;
    }

    if ( Memory != VK_NULL_HANDLE )
    {
        vkFreeMemory( DeviceWrapper.GetLogicalDevice(), Memory, nullptr );
        Memory = VK_NULL_HANDLE;
    }
}

void *LumenEngine::RHI::FVulkanBuffer::Map ( VkDeviceSize InSize, VkDeviceSize InOffset )
{
    void *DataPointer            = nullptr;
    FVulkanDevice &DeviceWrapper = FVulkanRHI::Get().GetDevice();
    VULKAN_CHECK( vkMapMemory( DeviceWrapper.GetLogicalDevice(), Memory, InOffset, InSize, 0, &DataPointer ) );
    return DataPointer;
}

void LumenEngine::RHI::FVulkanBuffer::Unmap ()
{
    FVulkanDevice &DeviceWrapper = FVulkanRHI::Get().GetDevice();
    vkUnmapMemory( DeviceWrapper.GetLogicalDevice(), Memory );
}
