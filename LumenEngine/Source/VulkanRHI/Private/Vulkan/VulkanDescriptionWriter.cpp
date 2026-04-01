/**
 * @file VulkanDescriptorWriter.cpp
 * @brief Implementation of the FVulkanDescriptorWriter class
 */

#include "Vulkan/VulkanDescriptorWriter.hpp"

void LumenEngine::VulkanRHI::FVulkanDescriptorWriter::WriteBuffer ( UInt32 Binding, VkBuffer Buffer, USize Size, USize Offset, VkDescriptorType Type )
{
    VkDescriptorBufferInfo &Info = BufferInfos.emplace_back( VkDescriptorBufferInfo{ .buffer = Buffer, .offset = Offset, .range = Size } );

    Writes.push_back( VkWriteDescriptorSet{ .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                            .pNext            = nullptr,
                                            .dstSet           = VK_NULL_HANDLE,
                                            .dstBinding       = Binding,
                                            .dstArrayElement  = 0,
                                            .descriptorCount  = 1,
                                            .descriptorType   = Type,
                                            .pImageInfo       = nullptr,
                                            .pBufferInfo      = &Info,
                                            .pTexelBufferView = nullptr } );
}

void LumenEngine::VulkanRHI::FVulkanDescriptorWriter::WriteImage (
    UInt32 Binding, VkImageView Image, VkSampler Sampler, VkImageLayout Layout, VkDescriptorType Type, UInt32 DstArrayElement )
{
    VkDescriptorImageInfo &Info = ImageInfos.emplace_back( VkDescriptorImageInfo{ .sampler = Sampler, .imageView = Image, .imageLayout = Layout } );

    Writes.push_back( VkWriteDescriptorSet{ .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                            .pNext            = nullptr,
                                            .dstSet           = VK_NULL_HANDLE,
                                            .dstBinding       = Binding,
                                            .dstArrayElement  = DstArrayElement,
                                            .descriptorCount  = 1,
                                            .descriptorType   = Type,
                                            .pImageInfo       = &Info,
                                            .pBufferInfo      = nullptr,
                                            .pTexelBufferView = nullptr } );
}

void LumenEngine::VulkanRHI::FVulkanDescriptorWriter::Clear ()
{
    BufferInfos.clear();
    ImageInfos.clear();
    Writes.clear();
}

void LumenEngine::VulkanRHI::FVulkanDescriptorWriter::UpdateSet ( VkDevice Device, VkDescriptorSet Set )
{
    for ( VkWriteDescriptorSet &Write : Writes )
    {
        Write.dstSet = Set;
    }
    vkUpdateDescriptorSets( Device, static_cast<UInt32>( Writes.size() ), Writes.data(), 0, nullptr );
}
