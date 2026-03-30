/**
 * @file VulkanBarrierBatcher.hpp
 * @brief Accumulates Vulkan memory and image barriers to submit them in batches.
 */

#pragma once

#include "Container/Vector.hpp"
#include "CoreTypes.hpp"
#include "VulkanCore.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    /**
     * @class FVulkanBarrierBatcher
     * @brief Optimizes pipeline barriers by batching them.
     */
    class FVulkanBarrierBatcher final
    {
    public:

        FVulkanBarrierBatcher () noexcept  = default;
        ~FVulkanBarrierBatcher () noexcept = default;

    public:

        /** Adds a memory barrier to the batch */
        void AddMemoryBarrier ( VkAccessFlags InSrcAccessMask, VkAccessFlags InDstAccessMask );

        /** Adds an image barrier to the batch for layout transitions */
        void AddImageBarrier ( VkImage InImage,
                               VkAccessFlags InSrcAccessMask,
                               VkAccessFlags InDstAccessMask,
                               VkImageLayout InOldLayout,
                               VkImageLayout InNewLayout,
                               VkImageAspectFlags InAspectMask );

        /** Adds a buffer barrier to the batch */
        void AddBufferBarrier ( VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InSize, VkAccessFlags InSrcAccessMask, VkAccessFlags InDstAccessMask );

        /**
         * @brief Submits all accumulated barriers into the provided command buffer.
         * @param InCmdBuffer The command buffer to record the barrier into.
         * @param InSrcStage The pipeline stage producing the data.
         * @param InDstStage The pipeline stage consuming the data.
         */
        void Flush ( VkCommandBuffer InCmdBuffer, VkPipelineStageFlags InSrcStage, VkPipelineStageFlags InDstStage );

        /** @brief Clears accumulated barriers without submitting */
        void Clear () noexcept;

    private:

        TVector<VkMemoryBarrier> MemoryBarriers;
        TVector<VkImageMemoryBarrier> ImageBarriers;
        TVector<VkBufferMemoryBarrier> BufferBarriers;
    };

} // namespace RHI

} // namespace LumenEngine

#include "Inline/VulkanBarrierBatcher.inl"
