/**
 * @file VulkanCommandPool.hpp
 * @brief Vulkan Command Pool RHI wrapper
 */

#pragma once

#include "Container/UniquePtr.hpp"
#include "Container/Vector.hpp"
#include "CoreTypes.hpp"
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    class FVulkanDevice;
    class FVulkanCommandBuffer;

    /**
     * @class FVulkanCommandPool
     * @brief Manages a VkCommandPool and its allocated VkCommandBuffers.
     */
    class FVulkanCommandPool final
    {
    public:

        FVulkanCommandPool ( FVulkanDevice &InDevice, UInt32 InQueueFamilyIndex ) noexcept;
        ~FVulkanCommandPool () noexcept;

    public:

        /**
         * @brief Allocates a new command buffer from the pool
         * @param bIsPrimary True for primary command buffers, false for secondary
         * @return A unique pointer to the wrapper FVulkanCommandBuffer
         */
        [[nodiscard]] TUniquePtr<FVulkanCommandBuffer> AllocateCommandBuffer ( Bool bIsPrimary = true );

        /** @brief Resets the pool, recycling all allocated command buffers efficiently */
        void Reset ();

    public:

        /** @return The underlying Vulkan command pool handle */
        [[nodiscard]] inline VkCommandPool GetHandle () const noexcept;

        /** @return The device that owns this pool */
        [[nodiscard]] inline FVulkanDevice &GetDevice () const noexcept;

    private:

        FVulkanDevice &Device;
        VkCommandPool Handle{ VK_NULL_HANDLE };
        UInt32 QueueFamilyIndex{ 0 };

        /** Tracks allocated command buffers to manage their memory */
        TVector<VkCommandBuffer> AllocatedBuffers;
    };

} // namespace RHI

} // namespace LumenEngine

#include "Inline/VulkanCommandPool.inl"
