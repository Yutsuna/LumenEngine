/**
 * @file VulkanRHI.hpp
 * @brief Main Render Hardware Interface for Vulkan backend.
 */

#pragma once

#include "Container/SharedPtr.hpp"
#include "CoreTypes.hpp"

#include "Vulkan/VulkanDevice.hpp"
#include "Vulkan/VulkanSwapChain.hpp"

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

namespace LumenEngine
{

class FGenericWindow;

namespace VulkanRHI
{

    /**
     * @class FVulkanRHI
     * @brief The main entry point and context manager for Vulkan rendering.
     */
    class LUMEN_ENGINE_API FVulkanRHI
    {
    public:

        FVulkanRHI ()  = default;
        ~FVulkanRHI () = default;

        /**
         * @brief Initializes the Vulkan instance, device, and swapchain.
         * @param InWindow The main application window to render to.
         */
        void Initialize ( const TSharedPtr<FGenericWindow> &InWindow );

        /**
         * @brief Cleans up all Vulkan resources.
         */
        void Shutdown ();

    public:

        void WaitIdle () const noexcept;
        bool BeginFrame ();
        void ClearScreen ( const Float32 ClearColor[4] );
        void EndFrame ();

    public:

        [[nodiscard]] FVulkanDevice GetDevice () const noexcept;
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice () const noexcept;
        [[nodiscard]] VkInstance GetInstance () const noexcept;
        [[nodiscard]] VmaAllocator GetAllocator () const noexcept;
        [[nodiscard]] FVulkanSwapChain &GetSwapChain () noexcept;

    private:

        void InitializeVulkanInstance ( const TSharedPtr<FGenericWindow> &InWindow );
        void InitializeVulkanDevice ();
        void InitializeVMA ();
        void InitializeSwapChain ( const TSharedPtr<FGenericWindow> &InWindow );
        void CreateCommandBuffers ();

    private:

        vkb::Instance Instance;
        vkb::PhysicalDevice PhysicalDevice;
        FVulkanDevice LogicalDevice;

        VkSurfaceKHR Surface   = VK_NULL_HANDLE;
        VmaAllocator Allocator = VK_NULL_HANDLE;

        FVulkanSwapChain SwapChain;

        Bool bIsInitialized = false;

        VkCommandPool CommandPool                         = VK_NULL_HANDLE;
        VkCommandBuffer CommandBuffers[MaxFramesInFlight] = {};

        UInt64 FrameIndex        = 0;
        UInt32 CurrentImageIndex = 0;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
