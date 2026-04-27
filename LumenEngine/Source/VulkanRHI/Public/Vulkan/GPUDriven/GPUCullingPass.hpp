/**
 * @file GPUCullingPass.hpp
 * @brief Compute pass that performs view-frustum culling entirely on the GPU.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Vulkan/VulkanCore.hpp"

#include "Shader/ShaderCompilerTypes.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    class FGPUSceneBuffer;
    class FGPUIndirectBuffer;

    /**
     * @class FGPUCullingPass
     * @brief Owns the VkPipeline and layout for the frustum-cull compute dispatch.
     */
    class LUMEN_ENGINE_API FGPUCullingPass final
    {
    public:

        FGPUCullingPass () noexcept  = default;
        ~FGPUCullingPass () noexcept = default;

    public:

        /**
         * @brief Creates the compute pipeline from the pre-compiled SPIR-V shader.
         *
         * Descriptor set layouts required (must be pre-created by FVulkanMemory
         * or similar):
         *   set=0  GlobalSetLayout  (UBO — already exists)
         *   set=1  SceneSetLayout   (scene SSBO)
         *   set=2  CullSetLayout    (indirect + count SSBOs)
         *
         * @param InDevice           Logical device.
         * @param InGlobalSetLayout  Existing set=0 layout from FVulkanMemory.
         * @param InSceneSetLayout   New set=1 layout for FGPUSceneBuffer.
         * @param InCullSetLayout    New set=2 layout for FGPUIndirectBuffer.
         * @param InSpirV            Pre-compiled SPIR-V shader.
         */
        Bool Initialize ( VkDevice InDevice,
                          VkDescriptorSetLayout InGlobalSetLayout,
                          VkDescriptorSetLayout InSceneSetLayout,
                          VkDescriptorSetLayout InCullSetLayout,
                          const FSpirVBlob &InSpirV );

        /**
         * @brief Destroys the compute pipeline and layout.
         * @param InDevice Logical device.
         */
        void Shutdown ( VkDevice InDevice ) noexcept;

        /**
         * @brief Records the full culling dispatch into InCmd.
         *
         * @param InCmd            Active command buffer.
         * @param InGlobalDescSet  set=0 descriptor set (global UBO).
         * @param InSceneBuffer    FGPUSceneBuffer for set=1 + instance count.
         * @param InIndirectBuffer FGPUIndirectBuffer for barriers + set=2.
         */
        void Execute ( VkCommandBuffer InCmd,
                       VkDescriptorSet InGlobalDescSet,
                       const FGPUSceneBuffer &InSceneBuffer,
                       const FGPUIndirectBuffer &InIndirectBuffer,
                       UInt32 InFrameIndex ) const noexcept;

        /** @return Whether Initialize() completed successfully. */
        [[nodiscard]] Bool IsReady () const noexcept;

    private:

        VkPipeline CullPipeline     = VK_NULL_HANDLE;
        VkPipelineLayout CullLayout = VK_NULL_HANDLE;

        /** Workgroup size must match the local_size_x in gpu_cull.comp. */
        static constexpr UInt32 WorkgroupSize = 64U;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
