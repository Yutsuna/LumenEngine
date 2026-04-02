/**
 * @file VulkanRHI.hpp
 * @brief Main Render Hardware Interface for Vulkan backend.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/SharedPtr.hpp"
#include "CoreTypes.hpp"

#include "Vulkan/VulkanBuffer.hpp"
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

        /** @brief Begins a dynamic rendering pass for the current frame */
        void BeginRendering ( const Float32 ClearColor[4] );

        /** @brief Ends the dynamic rendering pass */
        void EndRendering ();

        void EndFrame ();

    public:

        /** Resource Management Abstractions for the Renderer */

        [[nodiscard]] FVulkanBuffer CreateBuffer ( USize Size, VkBufferUsageFlags Usage, VmaMemoryUsage MemoryUsage );
        void DestroyBuffer ( FVulkanBuffer &Buffer );

        /** @brief Wrapper for binding vertex and index buffers without exposing raw Vulkan calls to Renderer */
        void BindMeshBuffers ( const FVulkanBuffer &VertexBuffer, const FVulkanBuffer &IndexBuffer );

        /** @brief Wrapper for draw call */
        void DrawIndexed ( UInt32 IndexCount );

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
