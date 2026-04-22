/**
 * @file GPUIndirectBuffer.hpp
 * @brief GPU buffer pair for indirect draw commands + draw count.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Vulkan/GPUDriven/GPUSceneBuffer.hpp"
#include "Vulkan/VulkanBuffer.hpp"
#include "Vulkan/VulkanCore.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FGPUIndirectBuffer
     * @brief Manages the indirect draw command buffer and the draw-count buffer.
     *
     * These buffers are GPU-only (DEVICE_LOCAL) because only the compute
     * shader writes to them and the graphics pipeline reads from them.
     * The culling compute shader uses BDA or a descriptor to write directly.
     */
    class LUMEN_ENGINE_API FGPUIndirectBuffer final
    {
    public:

        static constexpr USize MaxDraws = FGPUSceneBuffer::MaxInstances;

        FGPUIndirectBuffer () noexcept  = default;
        ~FGPUIndirectBuffer () noexcept = default;

    public:

        /**
         * @brief Allocates IndirectBuffer and CountBuffer on the GPU.
         *
         * @param InAllocator   VMA allocator.
         * @param InDevice      Logical device (for BDA queries if needed).
         * @param InDescPool    Descriptor pool for the write-set.
         * @param InCullSetLayout Layout for set=2 (the culling output set).
         */
        void Initialize ( VmaAllocator InAllocator, VkDevice InDevice, VkDescriptorPool InDescPool, VkDescriptorSetLayout InCullSetLayout );

        /**
         * @brief Destroys both GPU buffers.
         * @param InAllocator VMA allocator used at initialization.
         */
        void Shutdown ( VmaAllocator InAllocator ) noexcept;

        /**
         * @brief Inserts a pipeline barrier that ensures the indirect draw
         *        buffers are ready for compute writes.
         * @param InCmd The current frame's command buffer.
         */
        void InsertWriteBarrier ( VkCommandBuffer InCmd ) const noexcept;

        /**
         * @brief Inserts a barrier that makes the compute-written buffers
         *        visible to the indirect draw stage.
         * @param InCmd The current frame's command buffer.
         */
        void InsertReadBarrier ( VkCommandBuffer InCmd ) const noexcept;

    public:

        /** @return The VkBuffer containing VkDrawIndexedIndirectCommand[]. */
        [[nodiscard]] VkBuffer GetIndirectBuffer () const noexcept;

        /** @return The VkBuffer containing the surviving draw count (UInt32). */
        [[nodiscard]] VkBuffer GetCountBuffer () const noexcept;

        /** @return The descriptor set for set=2 used by the compute shader. */
        [[nodiscard]] VkDescriptorSet GetCullDescriptorSet () const noexcept;

    private:

        FVulkanBuffer IndirectBuffer; ///< VkDrawIndexedIndirectCommand * MaxDraws
        FVulkanBuffer CountBuffer;    ///< UInt32 draw count
        VkDescriptorSet CullDescSet = VK_NULL_HANDLE;
    };

} // namespace VulkanRHI

} // namespace LumenEngine