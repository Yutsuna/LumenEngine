/**
 * @file VulkanRHI.hpp
 * @brief Main Render Hardware Interface for Vulkan backend.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/SharedPtr.hpp"
#include "CoreTypes.hpp"

#include "RHI/RHI.hpp"
#include "RHI/RHICommandList.hpp"
#include "RHI/ResourceRegistry.hpp"

#include "Vulkan/GPUDriven/GPUCullingPass.hpp"
#include "Vulkan/GPUDriven/GPUIndirectBuffer.hpp"
#include "Vulkan/GPUDriven/GPUSceneBuffer.hpp"
#include "Vulkan/VulkanBuffer.hpp"
#include "Vulkan/VulkanCommandList.hpp"
#include "Vulkan/VulkanFrameContext.hpp"
#include "Vulkan/VulkanInstance.hpp"
#include "Vulkan/VulkanLogicalDevice.hpp"
#include "Vulkan/VulkanMemory.hpp"
#include "Vulkan/VulkanMesh.hpp"
#include "Vulkan/VulkanPhysicalDevice.hpp"
#include "Vulkan/VulkanPipeline.hpp"
#include "Vulkan/VulkanSwapChain.hpp"

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

        /** @brief Prepares the frame for rendering and updates global uniform buffers. */
        [[nodiscard]] Bool BeginFrame ( const RHI::FGlobalUniformData &InUniforms ) override;

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
        void PushConstantsInternal ( VkCommandBuffer InCmd, const RHI::FPipelineHandle InPipeline, const void *InData, UInt32 InSize, UInt32 InOffset ) noexcept;
        void DrawMeshInternal ( VkCommandBuffer InCmd, const RHI::FMeshHandle InMesh ) noexcept;
        void DrawSceneInternal ( VkCommandBuffer InCmd, const RHI::FSceneSnapshot &InSceneSnapshot, const Float32 InClearColor[4] ) noexcept;

    public:

        [[nodiscard]] FVulkanLogicalDevice GetLogicalDevice () const noexcept;
        [[nodiscard]] FVulkanPhysicalDevice GetPhysicalDevice () const noexcept;
        [[nodiscard]] FVulkanInstance GetInstance () const noexcept;
        [[nodiscard]] VmaAllocator GetAllocator () const noexcept;
        [[nodiscard]] FVulkanSwapChain &GetSwapChain () noexcept;

    private:

        void InitializeVulkanInstance ( const TSharedPtr<FGenericWindow> &InWindow );
        void InitializeVulkanDevice ();
        void InitializeSwapChain ( const TSharedPtr<FGenericWindow> &InWindow );

        void DestroyVulkanInstance () noexcept;
        void DestroyVulkanDevice () noexcept;
        void DestroySwapChain () noexcept;

        void InitializeGpuDrivenResources ();
        void ShutdownGpuDrivenResources () noexcept;

    private:

        RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> MeshRegistry;
        RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> PipelineRegistry;
        FVulkanCommandList CommandListImpl;

    private:

        FVulkanInstance Instance;
        FVulkanPhysicalDevice PhysicalDevice;
        FVulkanLogicalDevice LogicalDevice;
        FVulkanSwapChain SwapChain;
        FVulkanMemory Memory;
        FVulkanFrameContext FrameContext;

        FGPUSceneBuffer SceneBuffer;
        FGPUIndirectBuffer IndirectBuffer;
        FGPUCullingPass CullingPass;

        Bool bIsInitialized = false;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
