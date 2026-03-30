/**
 * @file VulkanSwapChain.hpp
 * @brief Vulkan SwapChain RHI wrapper
 */

#pragma once

#include "Container/Vector.hpp"
#include "CoreTypes.hpp"
#include "Maths/Vec.hpp"
#include "VulkanCore.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    class FVulkanQueue;
    class FVulkanDevice;
    class FVulkanSemaphore;

    /**
     * @enum ESwapChainStatus
     * @brief Enumeration for the status of the SwapChain
     */
    enum class ESwapChainStatus
    {
        Healthy     = 0,
        OutOfDate   = 1,
        SurfaceLost = 2,
    };

    /**
     * @class FVulkanSwapChain
     * @brief A wrapper class for Vulkan SwapChain, managing the presentation of rendered images to the screen.
     */
    class FVulkanSwapChain final
    {
    public:

        FVulkanSwapChain ( VkInstance InInstance,
                           FVulkanDevice &InDevice,
                           VkSurfaceKHR InSurface,
                           VkSwapchainKHR InSwapChain,
                           const Maths::FVec2u &InSize,
                           const UInt32 NumSwapChainImages ) noexcept;

        ~FVulkanSwapChain () noexcept;

    public:

        /**
         * @brief Acquires the next available image from the swap chain for rendering
         * @return The index of the acquired image in the swap chain
         */
        [[nodiscard]] UInt32 AcquireNextImage ();

        /**
         * @brief Presents the current image to the screen
         * @return ESwapChainStatus indicating the result of the present operation
         */
        [[nodiscard]] ESwapChainStatus Present ();

    public:

        /** @return The semaphore signaled when the current frame's image is acquired */
        [[nodiscard]] VkSemaphore GetImageAvailableSemaphore () const noexcept;

        /** @return The semaphore to be signaled when rendering is complete */
        [[nodiscard]] VkSemaphore GetRenderFinishedSemaphore () const noexcept;

        /** @return The fence to be signaled when the GPU finishes the current frame's commands */
        [[nodiscard]] VkFence GetInFlightFence () const noexcept;

        /** @return The handle to the underlying Vulkan swapchain */
        [[nodiscard]] VkSwapchainKHR GetHandle () const noexcept;

        /** @return The index of the currently acquired image */
        [[nodiscard]] UInt32 GetCurrentImageIndex () const noexcept;

    private:

        /** Validates the current swapchain state */
        void SwapChainValidation ();

        /** Destroys the swapchain and its associated resources */
        void Destroy () noexcept;

    private:

        static constexpr UInt32 InvalidSwapChainImageIndex = static_cast<UInt32>( -1 );

        /** Vulkan instance */
        VkInstance Instance{ VK_NULL_HANDLE };

        /** Vulkan device reference */
        FVulkanDevice &Device;

        /** Vulkan surface handle */
        VkSurfaceKHR Surface{ VK_NULL_HANDLE };

        /** Swapchain handle */
        VkSwapchainKHR Handle{ VK_NULL_HANDLE };

        /** Current image index */
        UInt32 CurrentImageIndex{ InvalidSwapChainImageIndex };

        /** Current frame in flight index (0 to MAX_FRAMES_IN_FLIGHT - 1) */
        UInt32 CurrentFrame{ 0 };

        /** Swapchain images */
        TVector<VkImage> Images;

        /** Synchronization resources for each frame in flight */
        struct FFrameSync
        {
            VkSemaphore ImageAvailableSemaphore{ VK_NULL_HANDLE };
            VkSemaphore RenderFinishedSemaphore{ VK_NULL_HANDLE };
            VkFence InFlightFence{ VK_NULL_HANDLE };
        } FrameSync[MAX_FRAMES_IN_FLIGHT];
    };

} // namespace RHI

} // namespace LumenEngine

#include "Inline/VulkanSwapChain.inl"
