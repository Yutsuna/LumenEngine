/**
 * @file VulkanSwapChain.hpp
 * @brief Declaration of the FVulkanSwapChain class for Vulkan presentation.
 */

#pragma once

#include "Container/Vector.hpp"
#include "Definitions.hpp"
#include "Maths/Vec.hpp"

#include <VkBootstrap.h>
#include <vulkan/vulkan_core.h>

#include "Vulkan/VulkanCore.hpp"

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @struct FFrameData
     * @brief Synchronization primitives for a single frame.
     */
    struct FFrameData final
    {
        VkSemaphore SwapChainSemaphore;
        VkSemaphore RenderSemaphore;
        VkFence RenderFence;
    };

    /**
     * @class FVulkanSwapChain
     * @brief Manages the Vulkan SwapChain and frame Synchronization
     */
    class LUMEN_ENGINE_API FVulkanSwapChain
    {
    public:

        FVulkanSwapChain ()  = default;
        ~FVulkanSwapChain () = default;

    public:

        /**
         * @brief Initializes synchronization primitives for the swap chain.
         * @param InDevice The Vulkan device to create synchronization objects with.
         */
        void InitializeSynStructures ( VkDevice InDevice );

        /**
         * @brief Creates the Vulkan SwapChain with the specified parameters.
         * @param InDevice The Vulkan device to create the swap chain with.
         * @param InSwapChainFormat The format of the swap chain images.
         * @param InSize The dimensions of the swap chain images.
         * @param bInVSyncEnabled Whether V-Sync should be enabled for the swap chain.
         */
        void Create ( const vkb::Device &InDevice, VkFormat InSwapChainFormat, const Maths::FVec2u &InSize, bool bInVSyncEnabled );

        /**
         * @brief Recreates the Vulkan SwapChain, typically in response to window resizing or format changes.
         * @param InDevice The Vulkan device to recreate the swap chain with.
         * @param InSwapChainFormat The new format of the swap chain images.
         * @param InSize The new dimensions of the swap chain images.
         * @param bInVSyncEnabled Whether V-Sync should be enabled for the new swap chain.
         */
        void Recreate ( const vkb::Device &InDevice, VkFormat InSwapChainFormat, const Maths::FVec2u &InSize, bool bInVSyncEnabled );

        /**
         * @brief Cleans up Vulkan resources associated with the swap chain.
         * @param InDevice The Vulkan device to clean up resources from.
         */
        void Cleanup ( VkDevice InDevice ) noexcept;

    public:

        [[nodiscard]] VkExtent2D GetExtent () const noexcept;
        [[nodiscard]] const TVector<VkImage> &GetImages () const noexcept;
        [[nodiscard]] VkImageView GetImageView ( const USize InImageIndex ) const noexcept;

        [[nodiscard]] Bool NeedsRecreation () const noexcept;

    public:

        /**
         * @brief Begins a new frame by waiting for the appropriate synchronization primitives to ensure that the previous frame has finished rendering.
         * @param InDevice The Vulkan device to wait on.
         * @param InFrameIndex The index of the current frame for synchronization purposes.
         */
        void BeginFrame ( VkDevice InDevice, USize InFrameIndex ) const;

        /**
         * @brief Resets the synchronization primitives for the current frame, allowing them to be reused for the next frame.
         * @param InDevice The Vulkan device to reset the fences on.
         * @param InFrameIndex The index of the current frame for synchronization purposes.
         */
        void ResetFences ( VkDevice InDevice, USize InFrameIndex ) const;

        /**
         * @brief Acquires the next available image from the swap chain for rendering.
         * @param InDevice The Vulkan device to acquire the image with.
         * @param InFrameIndex The index of the current frame for synchronization purposes.
         * @return A pair containing the acquired VkImage and its corresponding index in the swap chain
         */
        [[nodiscard]] std::pair<VkImage, UInt32> AcquireNextImage ( VkDevice InDevice, USize InFrameIndex );

        /**
         * @brief Submits the command buffer for execution and presents the rendered image to the screen.
         * @param InCmd The command buffer containing rendering commands to submit.
         * @param InGraphicsQueue The Vulkan graphics queue to submit the command buffer to.
         * @param InFrameIndex The index of the current frame for synchronization purposes.
         * @param InSwapChainImageIndex The index of the swap chain image to present.
         */
        void SubmitAndPresent ( VkCommandBuffer InCmd, VkQueue InGraphicsQueue, USize InFrameIndex, UInt32 InSwapChainImageIndex ) noexcept;

    private:

        vkb::Swapchain SwapChain;

        TVector<VkImage> Images;
        TVector<VkImageView> ImageViews;

        FFrameData Frames[MaxFramesInFlight] = {};
        Bool bIsDirty                        = false;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
