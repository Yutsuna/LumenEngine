/**
 * @file VulkanMemory.hpp
 * @brief Vulkan memory and global descriptor resources manager.
 */

#pragma once

#include "Definitions.hpp"

#include "CoreTypes.hpp"
#include "RHI/RHITypes.hpp"

#include "Vulkan/VulkanBuffer.hpp"
#include "Vulkan/VulkanCore.hpp"

#include <vk_mem_alloc.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FVulkanMemory
     * @brief Owns VMA allocator and per-frame global descriptor resources.
     */
    class LUMEN_ENGINE_API FVulkanMemory final
    {
    public:

        FVulkanMemory () noexcept  = default;
        ~FVulkanMemory () noexcept = default;

    public:

        /**
         * @brief Initializes the Vulkan memory manager.
         * @param InInstance The Vulkan instance.
         * @param InPhysicalDevice The physical device.
         * @param InDevice The logical device.
         */
        void Initialize ( VkInstance InInstance, VkPhysicalDevice InPhysicalDevice, VkDevice InDevice );

        /**
         * @brief Shuts down the Vulkan memory manager.
         * @param InDevice The logical device.
         */
        void Shutdown ( VkDevice InDevice ) noexcept;

        /**
         * @brief Updates the global uniform data for the specified frame.
         * @param InFrameIndex The frame index.
         * @param InUniforms The uniform data.
         */
        void UpdateGlobalUniformData ( UInt32 InFrameIndex, const RHI::FGlobalUniformData &InUniforms ) noexcept;

    public:

        [[nodiscard]] VmaAllocator GetAllocator () const noexcept;
        [[nodiscard]] VkDescriptorSetLayout GetGlobalSetLayout () const noexcept;
        [[nodiscard]] VkDescriptorSet GetGlobalDescriptorSet ( UInt32 InFrameIndex ) const noexcept;

    private:

        void InitializeVMA ( VkInstance InInstance, VkPhysicalDevice InPhysicalDevice, VkDevice InDevice );
        void InitializeDescriptors ( VkDevice InDevice );

        void DestroyDescriptors ( VkDevice InDevice ) noexcept;
        void DestroyVMA () noexcept;

    private:

        VkDescriptorSetLayout GlobalSetLayout                   = VK_NULL_HANDLE;
        VkDescriptorPool DescriptorPool                         = VK_NULL_HANDLE;
        FVulkanBuffer GlobalUniformBuffers[MaxFramesInFlight]   = {};
        VkDescriptorSet GlobalDescriptorSets[MaxFramesInFlight] = {};

        VmaAllocator Allocator = VK_NULL_HANDLE;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
