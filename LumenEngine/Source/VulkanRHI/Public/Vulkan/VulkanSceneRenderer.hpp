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
         * @brief Executes the rendering of a scene snapshot.
         *
         * @param InCmd             The active command buffer.
         * @param InSceneSnapshot   The data provided by the Game Thread.
         * @param InFrameIndex      Current frame-in-flight index.
         * @param InMeshRegistry    Reference to the RHI mesh storage.
         * @param InPipelineRegistry Reference to the RHI pipeline storage.
         * @param InMemory          The memory manager (for global descriptor sets).
         * @param InSceneBuffer     SSBO for instance data.
         * @param InIndirectBuffer  Indirect draw commands storage.
         * @param InCullingPass     The compute pass for GPU culling.
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
