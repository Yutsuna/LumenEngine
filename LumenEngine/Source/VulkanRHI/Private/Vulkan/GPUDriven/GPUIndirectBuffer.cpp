/**
 * @file GPUIndirectBuffer.cpp
 * @brief Implementation of FGPUIndirectBuffer.
 */

#include "Vulkan/GPUDriven/GPUIndirectBuffer.hpp"

#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanDescriptorWriter.hpp"

#include "Logging/Logger.hpp"

#include <vulkan/vulkan_core.h>

void LumenEngine::VulkanRHI::FGPUIndirectBuffer::Initialize ( VmaAllocator InAllocator,
                                                              VkDevice InDevice,
                                                              VkDescriptorPool InDescPool,
                                                              VkDescriptorSetLayout InCullSetLayout )
{
    {
        const VkDeviceSize IndirectSize = static_cast<VkDeviceSize>( MaxDraws * sizeof( VkDrawIndexedIndirectCommand ) );

        VkBufferCreateInfo BufInfo{};
        BufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufInfo.size  = IndirectSize;
        BufInfo.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
                        | VK_BUFFER_USAGE_TRANSFER_DST_BIT;                                      
        BufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo AllocCI{};
        AllocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        LUMEN_VK_CHECK( vmaCreateBuffer( InAllocator, &BufInfo, &AllocCI, &IndirectBuffer.Buffer, &IndirectBuffer.Allocation, &IndirectBuffer.AllocationInfo ) );
    }

    {
        VkBufferCreateInfo BufInfo{};
        BufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufInfo.size        = sizeof( UInt32 );
        BufInfo.usage       = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        BufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo AllocCI{};
        AllocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        LUMEN_VK_CHECK( vmaCreateBuffer( InAllocator, &BufInfo, &AllocCI, &CountBuffer.Buffer, &CountBuffer.Allocation, &CountBuffer.AllocationInfo ) );
    }

    {
        VkDescriptorSetAllocateInfo SetAllocInfo{};
        SetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        SetAllocInfo.descriptorPool     = InDescPool;
        SetAllocInfo.descriptorSetCount = 1;
        SetAllocInfo.pSetLayouts        = &InCullSetLayout;

        LUMEN_VK_CHECK( vkAllocateDescriptorSets( InDevice, &SetAllocInfo, &CullDescSet ) );

        FVulkanDescriptorWriter Writer;
        Writer.WriteBuffer( 0, IndirectBuffer.Buffer, static_cast<VkDeviceSize>( MaxDraws * sizeof( VkDrawIndexedIndirectCommand ) ), 0,
                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );

        Writer.WriteBuffer( 1, CountBuffer.Buffer, sizeof( UInt32 ), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );

        Writer.UpdateSet( InDevice, CullDescSet );
    }

    LUMEN_LOG_INFO( LogVulkanRHI, "GPUIndirectBuffer initialized: {} max indirect draws.", MaxDraws );
}

void LumenEngine::VulkanRHI::FGPUIndirectBuffer::Shutdown ( VmaAllocator InAllocator ) noexcept
{
    if ( IndirectBuffer.Buffer != VK_NULL_HANDLE )
    {
        vmaDestroyBuffer( InAllocator, IndirectBuffer.Buffer, IndirectBuffer.Allocation );
        IndirectBuffer.Buffer = VK_NULL_HANDLE;
    }

    if ( CountBuffer.Buffer != VK_NULL_HANDLE )
    {
        vmaDestroyBuffer( InAllocator, CountBuffer.Buffer, CountBuffer.Allocation );
        CountBuffer.Buffer = VK_NULL_HANDLE;
    }
}

void LumenEngine::VulkanRHI::FGPUIndirectBuffer::InsertWriteBarrier ( VkCommandBuffer InCmd ) const noexcept
{
    VkBufferMemoryBarrier2 Barriers[2] = {};

    Barriers[0].sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    Barriers[0].srcStageMask        = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    Barriers[0].srcAccessMask       = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    Barriers[0].dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    Barriers[0].dstAccessMask       = VK_ACCESS_2_SHADER_WRITE_BIT;
    Barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barriers[0].buffer              = IndirectBuffer.Buffer;
    Barriers[0].offset              = 0;
    Barriers[0].size                = VK_WHOLE_SIZE;

    Barriers[1]        = Barriers[0];
    Barriers[1].buffer = CountBuffer.Buffer;

    VkDependencyInfo DepInfo{};
    DepInfo.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    DepInfo.bufferMemoryBarrierCount = 2;
    DepInfo.pBufferMemoryBarriers    = Barriers;

    vkCmdPipelineBarrier2( InCmd, &DepInfo );
}

void LumenEngine::VulkanRHI::FGPUIndirectBuffer::InsertReadBarrier ( VkCommandBuffer InCmd ) const noexcept
{

    VkBufferMemoryBarrier2 Barriers[2] = {};

    Barriers[0].sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    Barriers[0].srcStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    Barriers[0].srcAccessMask       = VK_ACCESS_2_SHADER_WRITE_BIT;
    Barriers[0].dstStageMask        = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    Barriers[0].dstAccessMask       = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    Barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barriers[0].buffer              = IndirectBuffer.Buffer;
    Barriers[0].offset              = 0;
    Barriers[0].size                = VK_WHOLE_SIZE;

    Barriers[1]        = Barriers[0];
    Barriers[1].buffer = CountBuffer.Buffer;

    VkDependencyInfo DepInfo{};
    DepInfo.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    DepInfo.bufferMemoryBarrierCount = 2;
    DepInfo.pBufferMemoryBarriers    = Barriers;

    vkCmdPipelineBarrier2( InCmd, &DepInfo );
}

VkBuffer LumenEngine::VulkanRHI::FGPUIndirectBuffer::GetIndirectBuffer () const noexcept
{
    return IndirectBuffer.Buffer;
}

VkBuffer LumenEngine::VulkanRHI::FGPUIndirectBuffer::GetCountBuffer () const noexcept
{
    return CountBuffer.Buffer;
}

VkDescriptorSet LumenEngine::VulkanRHI::FGPUIndirectBuffer::GetCullDescriptorSet () const noexcept
{
    return CullDescSet;
}