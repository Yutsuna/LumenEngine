/**
 * @file VulkanBuffer.hpp
 * @brief Vulkan Buffer RHI wrapper
 */

#pragma once

#include "VulkanResource.hpp"
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    /**
     * @class FVulkanBuffer
     * @brief Wrapper for VkBuffer and its associated device memory
     */
    class FVulkanBuffer final : public FVulkanResource
    {
    public:

        FVulkanBuffer ( VkDeviceSize InSize, VkBufferUsageFlags InUsage, VkMemoryPropertyFlags InProperties ) noexcept;
        ~FVulkanBuffer () noexcept override;

    public:

        /** @brief Immediate release of the buffer and memory */
        void Release () override;

        /**
         * @brief Maps the buffer memory to a CPU-accessible pointer
         * @param InSize Size of the memory range to map
         * @param InOffset Offset within the buffer
         * @return Pointer to the mapped memory
         */
        [[nodiscard]] void *Map ( VkDeviceSize InSize = VK_WHOLE_SIZE, VkDeviceSize InOffset = 0 );

        /** @brief Unmaps the buffer memory */
        void Unmap ();

    public:

        /** @return The underlying Vulkan buffer handle */
        [[nodiscard]] inline VkBuffer GetHandle () const noexcept;

        /** @return The size of the buffer in bytes */
        [[nodiscard]] inline VkDeviceSize GetSize () const noexcept;

    private:

        /** Handle to the Vulkan buffer */
        VkBuffer Buffer{ VK_NULL_HANDLE };

        /** Handle to the allocated device memory */
        VkDeviceMemory Memory{ VK_NULL_HANDLE };

        /** Size of the buffer in bytes */
        VkDeviceSize Size{ 0 };
    };

} // namespace RHI

} // namespace LumenEngine

#include "Inline/VulkanBuffer.inl"
