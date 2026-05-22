/**
 * @file VulkanMemory.hpp
 * @brief Updated Vulkan memory and global descriptor resources manager.
 */

#pragma once

#include "Definitions.hpp"

#include "CoreTypes.hpp"
#include "RHI/RHITypes.hpp"

#include "Container/Vector.hpp"
#include "Vulkan/GPUDriven/GPUGlobalUniforms.hpp"
#include "Vulkan/VulkanBuffer.hpp"
#include "Vulkan/VulkanCore.hpp"

#include <vk_mem_alloc.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @struct FDescriptorConfig
     * @brief Runtime configuration for descriptors and uniform buffers.
     */
    struct FDescriptorConfig final
    {
        UInt32 MaxFramesInFlight = 0U;
    };

    /**
     * @class FVulkanMemory
     * @brief Owns VMA allocator, per-frame global UBOs, and all descriptor layouts.
     *
     * Descriptor set ownership:
     *   set=0  GlobalSetLayout / GlobalDescriptorSets  — owned here (UBO)
     *   set=1  SceneSetLayout                          — layout created here,
     *                                                    sets allocated by FGPUSceneBuffer
     *   set=2  CullSetLayout                           — layout created here,
     *                                                    set allocated by FGPUIndirectBuffer
     */
    class LUMEN_ENGINE_API FVulkanMemory final
    {
    public:

        FVulkanMemory () noexcept  = default;
        ~FVulkanMemory () noexcept = default;

    public:

        /**
         * @brief Initializes the Vulkan memory manager.
         * @param InInstance       The Vulkan instance.
         * @param InPhysicalDevice The physical device.
         * @param InDevice         The logical device.
         * @param InConfig         The descriptor configuration.
         */
        void Initialize ( VkInstance InInstance, VkPhysicalDevice InPhysicalDevice, VkDevice InDevice, const FDescriptorConfig &InConfig );

        /**
         * @brief Shuts down the Vulkan memory manager.
         * @param InDevice The logical device.
         */
        void Shutdown ( VkDevice InDevice ) noexcept;

        /**
         * @brief Uploads new global uniform data (including frustum planes) for
         *        the specified frame-in-flight slot.
         *
         * @param InFrameIndex The frame index [0, MaxFramesInFlight).
         * @param InUniforms   The full FGPUGlobalUniforms to upload.
         */
        void UpdateGlobalUniformData ( UInt32 InFrameIndex, const FGPUGlobalUniforms &InUniforms ) noexcept;

        /** @brief Legacy overload for code that still uses FGlobalUniformData. */
        void UpdateGlobalUniformData ( UInt32 InFrameIndex, const RHI::FGlobalUniformData &InUniforms ) noexcept;

    public:

        [[nodiscard]] VmaAllocator GetAllocator () const noexcept;
        [[nodiscard]] VkDescriptorPool GetDescriptorPool () const noexcept;
        [[nodiscard]] UInt32 GetNumFramesInFlight () const noexcept;

        /* set=0 */
        [[nodiscard]] VkDescriptorSetLayout GetGlobalSetLayout () const noexcept;
        [[nodiscard]] VkDescriptorSet GetGlobalDescriptorSet ( UInt32 InFrameIndex ) const noexcept;

        /* set=1 — layout only; sets are owned by FGPUSceneBuffer */
        [[nodiscard]] VkDescriptorSetLayout GetSceneSetLayout () const noexcept;

        /* set=2 — layout only; set is owned by FGPUIndirectBuffer */
        [[nodiscard]] VkDescriptorSetLayout GetCullSetLayout () const noexcept;

    private:

        void InitializeVMA ( VkInstance InInstance, VkPhysicalDevice InPhysicalDevice, VkDevice InDevice );

        void InitializeDescriptors ( VkDevice InDevice );

        void DestroyDescriptors ( VkDevice InDevice ) noexcept;
        void DestroyVMA () noexcept;

    private:

        /** set=0 — Global UBO */
        VkDescriptorSetLayout GlobalSetLayout = VK_NULL_HANDLE;
        TVector<VkDescriptorSet> GlobalDescriptorSets;
        TVector<FVulkanBuffer> GlobalUniformBuffers;

        /** set=1 — Scene SSBO layout (sets owned by FGPUSceneBuffer) */
        VkDescriptorSetLayout SceneSetLayout = VK_NULL_HANDLE;

        /** set=2 — Indirect draw output layout (set owned by FGPUIndirectBuffer) */
        VkDescriptorSetLayout CullSetLayout = VK_NULL_HANDLE;

        /** Shared pool — sized to cover all sets across all sub-systems */
        VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;

        VmaAllocator Allocator = VK_NULL_HANDLE;

        UInt32 NumFramesInFlight = 0U;
    };

} // namespace VulkanRHI

} // namespace LumenEngine