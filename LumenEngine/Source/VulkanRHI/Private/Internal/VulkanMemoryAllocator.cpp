/**
 * @file VulkanMemoryAllocator.cpp
 * @brief Implementation of a custom Vulkan memory allocator using a block-pooling strategy.
 */

#include "VulkanMemoryAllocator.hpp"
#include "VulkanCore.hpp"
#include "VulkanDevice.hpp"

LumenEngine::RHI::FVulkanMemoryAllocator::FVulkanMemoryAllocator ( FVulkanDevice &InDevice ) noexcept : Device( InDevice )
{
    /** Ctor */
}

LumenEngine::RHI::FVulkanMemoryAllocator::~FVulkanMemoryAllocator () noexcept
{
    for ( const FMemoryBlock &Block : Blocks )
    {
        vkFreeMemory( Device.GetLogicalDevice(), Block.Memory, nullptr );
    }
    Blocks.clear();
}

LumenEngine::TExpected<LumenEngine::RHI::FVulkanMemoryAllocation, LumenEngine::FString>
LumenEngine::RHI::FVulkanMemoryAllocator::Allocate ( VkDeviceSize InSize, VkMemoryRequirements InRequirements, VkMemoryPropertyFlags InProperties )
{
    std::lock_guard<std::mutex> Lock( AllocatorMutex );

    const UInt32 MemoryTypeIndex = FindMemoryType( InRequirements.memoryTypeBits, InProperties );

    /**
     * NOTE:
     * Simple block allocator logic (TODO: use VMA or a Buddy Allocator here)
     * For now, we allocate a new block for simplicity, but track it in the pool.
     */
    VkMemoryAllocateInfo AllocInfo{};
    AllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize  = InSize; /** TODO: In a real scenario, round up to 256MB chunks */
    AllocInfo.memoryTypeIndex = MemoryTypeIndex;

    VkDeviceMemory NewMemory = VK_NULL_HANDLE;
    VULKAN_CHECK( vkAllocateMemory( Device.GetLogicalDevice(), &AllocInfo, nullptr, &NewMemory ) );

    FMemoryBlock NewBlock{ .Memory = NewMemory, .TotalSize = InSize, .AllocatedSize = InSize, .Properties = InProperties, .MemoryTypeIndex = MemoryTypeIndex };
    Blocks.push_back( NewBlock );

    FVulkanMemoryAllocation Allocation{};
    Allocation.DeviceMemory = NewMemory;
    Allocation.Offset       = 0;
    Allocation.Size         = InSize;

    if ( static_cast<Bool>( InProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) )
    {
        VULKAN_CHECK( vkMapMemory( Device.GetLogicalDevice(), NewMemory, 0, InSize, 0, &Allocation.MappedPointer ) );
    }

    return Allocation;
}

void LumenEngine::RHI::FVulkanMemoryAllocator::Free ( const FVulkanMemoryAllocation &InAllocation )
{
    std::lock_guard<std::mutex> Lock( AllocatorMutex );

    /**
     * NOTE:
     * Simplification: In a real pooling allocator, we mark the offset as free.
     * Here we just free the block completely since we did 1:1 allocation above.
     * TODO: do better
     */
    using IteratorType = TVector<FMemoryBlock>::iterator;

    for ( IteratorType Iterator = Blocks.begin(); Iterator != Blocks.end(); ++Iterator )
    {
        if ( Iterator->Memory == InAllocation.DeviceMemory )
        {
            if ( InAllocation.MappedPointer != nullptr )
            {
                vkUnmapMemory( Device.GetLogicalDevice(), Iterator->Memory );
            }
            vkFreeMemory( Device.GetLogicalDevice(), Iterator->Memory, nullptr );
            Blocks.erase( Iterator );
            break;
        }
    }
}

LumenEngine::UInt32 LumenEngine::RHI::FVulkanMemoryAllocator::FindMemoryType ( UInt32 InTypeFilter, VkMemoryPropertyFlags InProperties ) const
{
    VkPhysicalDeviceMemoryProperties MemProperties;
    vkGetPhysicalDeviceMemoryProperties( Device.GetPhysicalDevice(), &MemProperties );

    for ( UInt32 Index = 0; Index < MemProperties.memoryTypeCount; ++Index )
    {
        const bool bMatchFilter        = ( InTypeFilter & ( 1 << Index ) ) != 0;
        const VkMemoryType &MemoryType = *( MemProperties.memoryTypes + Index );
        const bool bMatchProps         = ( MemoryType.propertyFlags & InProperties ) == InProperties;

        if ( bMatchFilter && bMatchProps )
        {
            return Index;
        }
    }

    LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to find suitable memory type!" );
    return static_cast<UInt32>( -1 );
}
