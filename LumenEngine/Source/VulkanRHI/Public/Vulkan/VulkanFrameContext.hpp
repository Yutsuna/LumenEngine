/**
 * @file VulkanFrameContext.hpp
 * @brief Frame lifecycle and command buffer context for Vulkan rendering.
 */

#pragma once

#include "Definitions.hpp"

#include "CoreTypes.hpp"

#include "Vulkan/VulkanCommandBuffer.hpp"
#include "Vulkan/VulkanCommandPool.hpp"
#include "Vulkan/VulkanSwapChain.hpp"

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FVulkanFrameContext
     * @brief Owns per-frame command buffers and frame/image indices.
     */
    class LUMEN_ENGINE_API FVulkanFrameContext final
    {
    public:

        FVulkanFrameContext () noexcept  = default;
        ~FVulkanFrameContext () noexcept = default;

    public:

        /**
         * @brief Initializes the frame context.
         * @param InDevice The Vulkan device.
         * @param InGraphicsQueueFamily The graphics queue family index.
         */
        void Initialize ( VkDevice InDevice, UInt32 InGraphicsQueueFamily );

        /**
         * @brief Shuts down the frame context.
         * @param InDevice The Vulkan device.
         */
        void Shutdown ( VkDevice InDevice ) noexcept;

        /**
         * @brief Begins a new frame by acquiring the next swapchain image and resetting command buffers.
         * @param InDevice The Vulkan device.
         * @param InSwapChain The swapchain to acquire images from.
         * @return True if the frame was successfully begun, false if the swapchain needs recreation
         */
        [[nodiscard]] Bool BeginFrame ( VkDevice InDevice, FVulkanSwapChain &InSwapChain );

        /**
         * @brief Submits command buffers and presents the frame.
         * @param InSwapChain The swapchain to present images to.
         * @param InGraphicsQueue The graphics queue to submit commands to.
         */
        void SubmitAndPresent ( FVulkanSwapChain &InSwapChain, VkQueue InGraphicsQueue );

    public:

        [[nodiscard]] UInt32 GetCurrentFrameIndex () const noexcept;
        [[nodiscard]] UInt32 GetCurrentImageIndex () const noexcept;

        [[nodiscard]] FVulkanCommandBuffer &GetCurrentCommandBuffer () noexcept;
        [[nodiscard]] const FVulkanCommandBuffer &GetCurrentCommandBuffer () const noexcept;

    private:

        FVulkanCommandPool CommandPool;
        FVulkanCommandBuffer CommandBuffers[MaxFramesInFlight];

        UInt64 FrameIndex        = 0;
        UInt32 CurrentImageIndex = 0;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
