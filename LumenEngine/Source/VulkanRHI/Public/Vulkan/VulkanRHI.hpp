/**
 * @file VulkanRHI.hpp
 * @brief Main Render Hardware Interface for Vulkan backend.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/Map.hpp"
#include "Container/SharedPtr.hpp"
#include "CoreTypes.hpp"

#include "RHI/RHI.hpp"
#include "RHI/RHICommandList.hpp"
#include "Vulkan/VulkanCommandList.hpp"

#include "Maths/Vertex.hpp"
#include "Vulkan/VulkanBuffer.hpp"
#include "Vulkan/VulkanCommandBuffer.hpp"
#include "Vulkan/VulkanCommandPool.hpp"
#include "Vulkan/VulkanInstance.hpp"
#include "Vulkan/VulkanLogicalDevice.hpp"
#include "Vulkan/VulkanMesh.hpp"
#include "Vulkan/VulkanPhysicalDevice.hpp"
#include "Vulkan/VulkanPipeline.hpp"
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
    class LUMEN_ENGINE_API FVulkanRHI final : public RHI::IRHI
    {
    public:

        FVulkanRHI () noexcept;
        ~FVulkanRHI () noexcept override = default;

    public:

        /**
         * @brief Initializes the Vulkan instance, device, and swapchain.
         * @param InWindow The main application window to render to.
         */
        void Initialize ( const TSharedPtr<FGenericWindow> &InWindow ) override;

        /** @brief Cleans up all Vulkan resources. */
        void Shutdown () override;

        /** @brief Waits until the logical device is idle. */
        void WaitIdle () const noexcept override;

        /** @brief Prepares the frame for rendering. */
        [[nodiscard]] Bool BeginFrame () override;

        /** @brief Submits the frame and presents it. */
        void EndFrame () override;

    public:

        /** @brief Retrieves the command list for recording agnostic RHI commands. */
        [[nodiscard]] RHI::IRHICommandList &GetCommandList () noexcept override;

        /**
         * @brief Creates a new mesh with the given vertices and indices.
         * @param InVertices The vertices of the mesh.
         * @param InIndices The indices of the mesh.
         * @return A strongly typed handle to the created mesh.
         */
        [[nodiscard]] RHI::FMeshHandle CreateMesh ( const TVector<Maths::FVertex> &InVertices, const TVector<UInt32> &InIndices ) override;

        /**
         * @brief Creates a new graphics pipeline with the given vertex and fragment shaders.
         * @param InVertexPath The path to the vertex shader file.
         * @param InFragmentPath The path to the fragment shader file.
         * @return A strongly typed handle to the created pipeline.
         */
        [[nodiscard]] RHI::FPipelineHandle CreatePipeline ( const FString &InVertexPath, const FString &InFragmentPath ) override;

    public:

        /**
         * Internal functions used by FVulkanCommandList to translate RHI calls into Vulkan calls.
         */
        void BeginRenderingInternal ( VkCommandBuffer InCmd, const Float32 InClearColor[4] ) noexcept;
        void BindPipelineInternal ( VkCommandBuffer InCmd, const RHI::FPipelineHandle InPipeline ) noexcept;
        void DrawMeshInternal ( VkCommandBuffer InCmd, const RHI::FMeshHandle InMesh ) noexcept;

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

        void DestroyVulkanInstance () noexcept;
        void DestroyVulkanDevice () noexcept;
        void DestroyVMA () noexcept;
        void DestroySwapChain () noexcept;
        void DestroyCommandBuffers () noexcept;

    private:

        TMap<UInt32, FVulkanMesh> MeshRegistry;
        TMap<UInt32, FVulkanPipeline> PipelineRegistry;
        FVulkanCommandList CommandListImpl;

        UInt32 NextMeshID     = 1;
        UInt32 NextPipelineID = 1;

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