/**
 * @file VulkanDescriptorWriter.hpp
 * @brief Declaration of the FVulkanDescriptorWriter class
 */

#include "Container/Vector.hpp"
#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include <deque>
#include <vulkan/vulkan_core.h>

#pragma once

namespace LumenEngine
{

namespace VulkanRHI
{
    class LUMEN_ENGINE_API FVulkanDescriptorWriter final
    {

    public:

        FVulkanDescriptorWriter () noexcept  = default;
        ~FVulkanDescriptorWriter () noexcept = default;

    public:

        /**
         * @brief Adds a buffer descriptor write operation to the writer.
         * @param Binding The binding index in the descriptor set layout.
         * @param Buffer The Vulkan buffer to bind.
         * @param Size The size of the buffer region to bind.
         * @param Offset The offset within the buffer to start binding from.
         * @param Type The type of the descriptor (e.g., VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER).
         * @param DstArrayElement The starting array element index for array descriptors (default is 0).
         */
        void WriteBuffer ( UInt32 Binding, VkBuffer Buffer, USize Size, USize Offset, VkDescriptorType Type );

        /**
         * @brief Adds an image descriptor write operation to the writer.
         * @param Binding The binding index in the descriptor set layout.
         * @param Image The Vulkan image view to bind.
         * @param Sampler The Vulkan sampler to bind (if applicable).
         * @param Layout The layout of the image (e.g., VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL).
         * @param Type The type of the descriptor (e.g., VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER).
         * @param DstArrayElement The starting array element index for array descriptors (default is 0).
         */
        void WriteImage ( UInt32 Binding, VkImageView Image, VkSampler Sampler, VkImageLayout Layout, VkDescriptorType Type, UInt32 DstArrayElement = 0 );

        /**
         * @brief Clears all pending descriptor write operations from the writer.
         * This can be used to reset the writer before recording a new set of descriptor updates.
         */
        void Clear ();

        /**
         * @brief Applies all recorded descriptor write operations to the specified Vulkan descriptor set.
         * @param Device The Vulkan device associated with the descriptor set.
         * @param Set The Vulkan descriptor set to update with the recorded write operations.
         */
        void UpdateSet ( VkDevice Device, VkDescriptorSet Set );

    private:

        std::deque<VkDescriptorBufferInfo> BufferInfos;
        std::deque<VkDescriptorImageInfo> ImageInfos;
        TVector<VkWriteDescriptorSet> Writes;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
