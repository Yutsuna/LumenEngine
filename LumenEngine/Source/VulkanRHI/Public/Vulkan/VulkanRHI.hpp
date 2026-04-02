/**
 * @file VulkanRHI.hpp
 * @brief Main Render Hardware Interface for Vulkan backend.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/SharedPtr.hpp"
#include "CoreTypes.hpp"

#include "Vulkan/VulkanCommandBuffer.hpp"
#include "Vulkan/VulkanCommandPool.hpp"
#include "Vulkan/VulkanInstance.hpp"
#include "Vulkan/VulkanLogicalDevice.hpp"
#include "Vulkan/VulkanPhysicalDevice.hpp"
#include "Vulkan/VulkanSwapChain.hpp"

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

        [[nodiscard]] FVulkanLogicalDevice GetLogicalDevice () const noexcept;
        [[nodiscard]] FVulkanPhysicalDevice GetPhysicalDevice () const noexcept;
        [[nodiscard]] FVulkanInstance GetInstance () const noexcept;
        [[nodiscard]] VmaAllocator GetAllocator () const noexcept;
        [[nodiscard]] FVulkanSwapChain &GetSwapChain () noexcept;

    private:

        void InitializeVulkanInstance ( const TSharedPtr<FGenericWindow> &InWindow );
        void InitializeVulkanDevice ();
        void InitializeVMA ();
        void InitializeSwapChain ( const TSharedPtr<FGenericWindow> &InWindow );
        void InitializeCommandBuffers ();

        void DestroyVulkanInstance ();
        void DestroyVulkanDevice ();
        void DestroyVMA ();
        void DestroySwapChain ();
        void DestroyCommandBuffers ();

    private:

        FVulkanInstance Instance;
        FVulkanPhysicalDevice PhysicalDevice;
        FVulkanLogicalDevice LogicalDevice;
        FVulkanSwapChain SwapChain;

        FVulkanCommandPool CommandPool;
        FVulkanCommandBuffer CommandBuffers[MaxFramesInFlight];

        VmaAllocator Allocator = VK_NULL_HANDLE;

        Bool bIsInitialized = false;

        UInt64 FrameIndex        = 0;
        UInt32 CurrentImageIndex = 0;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
