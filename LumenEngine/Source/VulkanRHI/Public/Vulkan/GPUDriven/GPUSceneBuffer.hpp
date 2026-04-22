/**
 * @file GPUSceneBuffer.hpp
 * @brief Per-frame GPU Storage Buffer holding all instance data for the scene.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Container/Vector.hpp"

#include "Vulkan/GPUDriven/GPUSceneTypes.hpp"
#include "Vulkan/VulkanBuffer.hpp"
#include "Vulkan/VulkanCore.hpp"

#include <vk_mem_alloc.h>

namespace LumenEngine
{

namespace Engine
{
    struct FSpatialRegistryData;
}

namespace RHI
{

    template <typename ResourceType, typename Tag> class TResourceRegistry;
    struct FMeshTag;
    struct FPipelineTag;

} // namespace RHI

namespace VulkanRHI
{

    class FVulkanMesh;
    class FVulkanPipeline;

    /**
     * @class FGPUSceneBuffer
     * @brief Manages the per-frame SSBO for GPU-visible instance data.
     */
    class LUMEN_ENGINE_API FGPUSceneBuffer final
    {
    public:

        /** Maximum instances supported without reallocation. */
        static constexpr USize MaxInstances = 65536ULL;

        FGPUSceneBuffer () noexcept  = default;
        ~FGPUSceneBuffer () noexcept = default;

    public:

        /**
         * @brief Allocates the per-frame SSBOs and creates descriptor sets.
         *
         * @param InAllocator     VMA allocator for buffer creation.
         * @param InDevice        Logical device for descriptor operations.
         * @param InDescPool      Pool from which descriptor sets are allocated.
         * @param InSceneSetLayout The VkDescriptorSetLayout for set=1.
         */
        void Initialize ( VmaAllocator InAllocator, VkDevice InDevice, VkDescriptorPool InDescPool, VkDescriptorSetLayout InSceneSetLayout );

        /**
         * @brief Frees all GPU resources.
         * @param InAllocator VMA allocator used during Initialize().
         * @param InDevice    Logical device.
         */
        void Shutdown ( VmaAllocator InAllocator, VkDevice InDevice ) noexcept;

        /**
         * @brief Translates a snapshot from FSpatialRegistry into GPU instance data
         *        and uploads it to the current frame's SSBO.
         * @param InSnapshot      Read-only snapshot from FSpatialRegistry.
         * @param MeshRegistry    To resolve FMeshHandle → FVulkanMesh.
         * @param PipelineRegistry To resolve FPipelineHandle → FVulkanPipeline.
         * @param InFrameIndex    Current frame-in-flight index [0, MaxFramesInFlight).
         * @return Number of valid instances written to the GPU buffer.
         */
        UInt32 Upload ( const Engine::FSpatialRegistryData &InSnapshot,
                        const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &MeshRegistry,
                        const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &PipelineRegistry,
                        UInt32 InFrameIndex );

    public:

        /** @return The SSBO VkBuffer for the given frame index. */
        [[nodiscard]] VkBuffer GetBuffer ( UInt32 InFrameIndex ) const noexcept;

        /** @return The descriptor set for set=1 for the given frame index. */
        [[nodiscard]] VkDescriptorSet GetDescriptorSet ( UInt32 InFrameIndex ) const noexcept;

        /** @return Number of instances written in the last Upload() call. */
        [[nodiscard]] UInt32 GetLastInstanceCount () const noexcept;

    private:

        FVulkanBuffer SSBOs[MaxFramesInFlight]            = {};
        VkDescriptorSet DescriptorSets[MaxFramesInFlight] = {};

        /** Number of valid instances from the last Upload() call. */
        UInt32 LastInstanceCount = 0U;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
