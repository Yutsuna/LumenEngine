/**
 * @file VulkanMemoryAllocator.hpp
 * @brief Custom Vulkan memory allocator implementing a block-pooling strategy.
 */

#pragma once

#include "Container/String.hpp"
#include "Container/Vector.hpp"
#include "CoreTypes.hpp"

#include <mutex>
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    class FVulkanDevice;

    /**
     * @struct FVulkanMemoryAllocation
     * @brief Represents a memory allocation, including the device memory handle, offset, size, and mapped pointer.
     */
    struct FVulkanMemoryAllocation
    {
        VkDeviceMemory DeviceMemory{ VK_NULL_HANDLE };
        VkDeviceSize Offset{ 0 };
        VkDeviceSize Size{ 0 };
        void *MappedPointer{ nullptr };
    };

    /**
     * @class FVulkanMemoryAllocator
     * @brief Manages large VkDeviceMemory blocks and sub-allocates them.
     */
    class FVulkanMemoryAllocator final
    {

    public:

        FVulkanMemoryAllocator ( FVulkanDevice &InDevice ) noexcept;
        ~FVulkanMemoryAllocator () noexcept;

    public:

        /**
         * @brief Allocates memory based on the specified size, requirements, and properties.
         * @param InSize The size of the memory to allocate.
         * @param InRequirements The memory requirements for the allocation.
         * @param InProperties The desired memory properties (e.g., host visible, device local).
         * @return A result containing either a successful memory allocation or an error message.
         */
        TExpected<FVulkanMemoryAllocation, FString> Allocate ( VkDeviceSize InSize, VkMemoryRequirements InRequirements, VkMemoryPropertyFlags InProperties );
        void Free ( const FVulkanMemoryAllocation &InAllocation );

    private:

        /**
         * @brief Finds a suitable memory type index based on the provided type filter and properties.
         * @param InTypeFilter A bitmask specifying the acceptable memory types.
         * @param InProperties The desired memory properties (e.g., host visible, device local).
         * @return The index of a suitable memory type that matches the criteria.
         */
        [[nodiscard]] UInt32 FindMemoryType ( UInt32 InTypeFilter, VkMemoryPropertyFlags InProperties ) const;

    private:

        FVulkanDevice &Device;

        struct FMemoryBlock
        {
            VkDeviceMemory Memory;
            VkDeviceSize TotalSize;
            VkDeviceSize AllocatedSize;
            VkMemoryPropertyFlags Properties;
            UInt32 MemoryTypeIndex;
        };

        TVector<FMemoryBlock> Blocks;
        std::mutex AllocatorMutex;
    };

} // namespace RHI

} // namespace LumenEngine
