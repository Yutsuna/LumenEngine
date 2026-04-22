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
         * @brief Orchestrates the rendering sequence for the provided scene snapshot.
         *
         * @param InCmd             The command buffer currently recording.
         * @param InSceneSnapshot   Snapshot of entities (transforms/meshes/shaders) to draw.
         * @param InFrameIndex      Index of the current frame in flight [0, MaxFramesInFlight).
         * @param InMeshRegistry    Registry used to resolve backend Mesh resources.
         * @param InPipelineRegistry Registry used to resolve backend Pipeline resources.
         * @param InMemory          Global memory manager for UBO access.
         * @param InSceneBuffer     SSBO used for instance data streaming.
         * @param InIndirectBuffer  Buffers containing indirect commands and draw count.
         * @param InCullingPass     The Compute Pipeline responsible for GPU culling.
         */
        void RenderScene ( VkCommandBuffer InCmd,
                           const RHI::FSceneSnapshot &InSceneSnapshot,
                           UInt32 InFrameIndex,
                           const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
                           const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                           const FVulkanMemory &InMemory,
                           FGPUSceneBuffer &InSceneBuffer,
                           const FGPUIndirectBuffer &InIndirectBuffer,
                           const FGPUCullingPass &InCullingPass ) noexcept;

    } // namespace VulkanSceneRenderer

} // namespace VulkanRHI

} // namespace LumenEngine