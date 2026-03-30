/**
 * @file VulkanCommandBuffer.hpp
 * @brief Vulkan Command Buffer RHI wrapper
 */

#pragma once

#include "CoreTypes.hpp"
#include "VulkanBarrierBatcher.hpp"
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    class FVulkanCommandPool;

    /**
     * @class FVulkanCommandBuffer
     * @brief Wraps a VkCommandBuffer and tracks its recording state.
     */
    class FVulkanCommandBuffer final
    {
    public:

        FVulkanCommandBuffer ( FVulkanCommandPool &InPool, VkCommandBuffer InHandle ) noexcept;
        ~FVulkanCommandBuffer () noexcept = default;

    public:

        /** @brief Begins command buffer recording */
        void Begin ( VkCommandBufferUsageFlags InUsageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT );

        /** @brief Ends command buffer recording */
        void End ();

    public:

        /** @return The underlying Vulkan command buffer handle */
        [[nodiscard]] inline VkCommandBuffer GetHandle () const noexcept;

        /** @return The barrier batcher attached to this command buffer */
        [[nodiscard]] inline FVulkanBarrierBatcher &GetBarrierBatcher () noexcept;

        /** @return True if the command buffer is currently recording */
        [[nodiscard]] inline Bool IsRecording () const noexcept;

    private:

        FVulkanCommandPool &ParentPool;
        VkCommandBuffer Handle{ VK_NULL_HANDLE };
        FVulkanBarrierBatcher BarrierBatcher;

        Bool bIsRecording{ false };
    };

} // namespace RHI

} // namespace LumenEngine

#include "Inline/VulkanCommandBuffer.inl"
