/**
 * @file VulkanSceneRenderer.hpp
 * @brief Sub-system responsible for translating scene snapshots into Vulkan draw calls.
 */

#pragma once

#include "CoreTypes.hpp"
#include "RHI/RHITypes.hpp"
#include "RHI/ResourceRegistry.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    class FVulkanPipeline;
    class FVulkanMesh;
    class FVulkanMemory;
    class FGPUSceneBuffer;
    class FGPUIndirectBuffer;
    class FGPUCullingPass;

    /**
     * @namespace VulkanSceneRenderer
     * @brief Encapsulates the logic for rendering a scene snapshot, including both CPU and GPU-driven paths.
     */
    namespace VulkanSceneRenderer
    {

        /**
         * @brief Phase 1: Preparation.
         *        Handles data upload and Compute Culling.
         *        MUST be called OUTSIDE of a BeginRendering/EndRendering block.
         */
        void PrepareScene ( VkCommandBuffer InCmd,
                            const RHI::FSceneSnapshot &InSceneSnapshot,
                            UInt32 InFrameIndex,
                            const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
                            const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                            const FVulkanMemory &InMemory,
                            FGPUSceneBuffer &InSceneBuffer,
                            const FGPUIndirectBuffer &InIndirectBuffer,
                            const FGPUCullingPass &InCullingPass ) noexcept;

        /**
         * @brief Records actual draw commands (Indirect or Fallback).
         *        MUST be called INSIDE a BeginRendering/EndRendering block.
         */
        void RenderScene ( VkCommandBuffer InCmd,
                           const RHI::FSceneSnapshot &InSceneSnapshot,
                           UInt32 InFrameIndex,
                           const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
                           const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                           const FVulkanMemory &InMemory,
                           const FGPUIndirectBuffer &InIndirectBuffer,
                           const FGPUCullingPass &InCullingPass ) noexcept;

    } // namespace VulkanSceneRenderer

} // namespace VulkanRHI

} // namespace LumenEngine