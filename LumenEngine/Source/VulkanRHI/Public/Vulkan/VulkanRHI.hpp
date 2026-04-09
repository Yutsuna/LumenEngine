/**
 * @file VulkanRHI.hpp
 * @brief Main Render Hardware Interface for Vulkan backend.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/SharedPtr.hpp"
#include "Container/Map.hpp"
#include "CoreTypes.hpp"

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

        /** @brief Cleans up all Vulkan resources. */
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

        /**
         * @brief Creates a new mesh with the given vertices and indices.
         * @param InVertices The vertices of the mesh.
         * @param InIndices The indices of the mesh.
         * @return The ID of the created mesh.
         */
        [[nodiscard]] UInt32 CreateMesh ( const TVector<Maths::FVertex> &InVertices, const TVector<UInt32> &InIndices );

        /**
         * @brief Creates a new graphics pipeline with the given vertex and fragment shaders.
         * @param InVertexPath The path to the vertex shader file.
         * @param InFragmentPath The path to the fragment shader file.
         * @return The ID of the created pipeline.
         */
        [[nodiscard]] UInt32 CreatePipeline ( const FString &InVertexPath, const FString &InFragmentPath );

        /** @brief Binds the specified pipeline for rendering */
        void BindPipeline ( UInt32 PipelineID );

        /** @brief Draws the specified mesh */
        void DrawMesh ( UInt32 MeshID );

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

        TMap<UInt32, FVulkanMesh> MeshRegistry;
        TMap<UInt32, FVulkanPipeline> PipelineRegistry;

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