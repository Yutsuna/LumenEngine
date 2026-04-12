/**
 * @file VulkanMemory.cpp
 * @brief Implementation of Vulkan memory and global descriptors manager.
 */

#include "Vulkan/VulkanMemory.hpp"

#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanDescriptorWriter.hpp"

#include <cstring>

namespace
{

VkDescriptorSetLayoutBinding CreateGlobalUniformLayoutBinding () noexcept
{
    return {
        .binding            = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };
}

void CreateDescriptorSetLayout ( VkDevice InDevice, VkDescriptorSetLayout &OutLayout )
{
    const VkDescriptorSetLayoutBinding Binding = CreateGlobalUniformLayoutBinding();

    VkDescriptorSetLayoutCreateInfo LayoutInfo{};
    LayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutInfo.bindingCount = 1;
    LayoutInfo.pBindings    = &Binding;

    LUMEN_VK_CHECK( vkCreateDescriptorSetLayout( InDevice, &LayoutInfo, nullptr, &OutLayout ) );
}

void CreateDescriptorPool ( VkDevice InDevice, VkDescriptorPool &OutPool )
{
    VkDescriptorPoolSize PoolSize{};
    PoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    PoolSize.descriptorCount = static_cast<LumenEngine::UInt32>( LumenEngine::VulkanRHI::MaxFramesInFlight );

    VkDescriptorPoolCreateInfo PoolInfo{};
    PoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = 1;
    PoolInfo.pPoolSizes    = &PoolSize;
    PoolInfo.maxSets       = static_cast<LumenEngine::UInt32>( LumenEngine::VulkanRHI::MaxFramesInFlight );

    LUMEN_VK_CHECK( vkCreateDescriptorPool( InDevice, &PoolInfo, nullptr, &OutPool ) );
}

void CreateGlobalUniformBuffers ( VmaAllocator InAllocator, LumenEngine::VulkanRHI::FVulkanBuffer OutBuffers[LumenEngine::VulkanRHI::MaxFramesInFlight] )
{
    VkBufferCreateInfo BufferInfo{};
    BufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size        = sizeof( LumenEngine::RHI::FGlobalUniformData );
    BufferInfo.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo AllocInfo{};
    AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    AllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    for ( LumenEngine::UInt32 Index = 0; Index < LumenEngine::VulkanRHI::MaxFramesInFlight; ++Index )
    {
        LUMEN_VK_CHECK(
            vmaCreateBuffer( InAllocator, &BufferInfo, &AllocInfo, &OutBuffers[Index].Buffer, &OutBuffers[Index].Allocation, &OutBuffers[Index].AllocationInfo ) );
    }
}

void AllocateDescriptorSets ( VkDevice InDevice,
                              VkDescriptorPool InPool,
                              VkDescriptorSetLayout InLayout,
                              VkDescriptorSet OutDescriptorSets[LumenEngine::VulkanRHI::MaxFramesInFlight] )
{
    LumenEngine::TVector<VkDescriptorSetLayout> Layouts( LumenEngine::VulkanRHI::MaxFramesInFlight, InLayout );

    VkDescriptorSetAllocateInfo AllocSetInfo{};
    AllocSetInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocSetInfo.descriptorPool     = InPool;
    AllocSetInfo.descriptorSetCount = static_cast<LumenEngine::UInt32>( LumenEngine::VulkanRHI::MaxFramesInFlight );
    AllocSetInfo.pSetLayouts        = Layouts.data();

    LUMEN_VK_CHECK( vkAllocateDescriptorSets( InDevice, &AllocSetInfo, OutDescriptorSets ) );
}

void UpdateDescriptorSets ( VkDevice InDevice,
                            const LumenEngine::VulkanRHI::FVulkanBuffer InBuffers[LumenEngine::VulkanRHI::MaxFramesInFlight],
                            const VkDescriptorSet InDescriptorSets[LumenEngine::VulkanRHI::MaxFramesInFlight] )
{
    for ( LumenEngine::UInt32 Index = 0; Index < LumenEngine::VulkanRHI::MaxFramesInFlight; ++Index )
    {
        LumenEngine::VulkanRHI::FVulkanDescriptorWriter Writer;
        Writer.WriteBuffer( 0, InBuffers[Index].Buffer, sizeof( LumenEngine::RHI::FGlobalUniformData ), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER );
        Writer.UpdateSet( InDevice, InDescriptorSets[Index] );
    }
}

} // namespace

void LumenEngine::VulkanRHI::FVulkanMemory::Initialize ( VkInstance InInstance, VkPhysicalDevice InPhysicalDevice, VkDevice InDevice )
{
    InitializeVMA( InInstance, InPhysicalDevice, InDevice );
    InitializeDescriptors( InDevice );
}

void LumenEngine::VulkanRHI::FVulkanMemory::Shutdown ( VkDevice InDevice ) noexcept
{
    DestroyDescriptors( InDevice );
    DestroyVMA();
}

void LumenEngine::VulkanRHI::FVulkanMemory::InitializeVMA ( VkInstance InInstance, VkPhysicalDevice InPhysicalDevice, VkDevice InDevice )
{
    VmaVulkanFunctions VulkanFunctions    = {};
    VulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    VulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo AllocatorInfo = {};
    AllocatorInfo.physicalDevice         = InPhysicalDevice;
    AllocatorInfo.device                 = InDevice;
    AllocatorInfo.instance               = InInstance;
    AllocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    AllocatorInfo.pVulkanFunctions       = &VulkanFunctions;
    AllocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    LUMEN_VK_CHECK( vmaCreateAllocator( &AllocatorInfo, &Allocator ) );
}

void LumenEngine::VulkanRHI::FVulkanMemory::InitializeDescriptors ( VkDevice InDevice )
{
    CreateDescriptorSetLayout( InDevice, GlobalSetLayout );
    CreateDescriptorPool( InDevice, DescriptorPool );
    CreateGlobalUniformBuffers( Allocator, GlobalUniformBuffers );
    AllocateDescriptorSets( InDevice, DescriptorPool, GlobalSetLayout, GlobalDescriptorSets );
    UpdateDescriptorSets( InDevice, GlobalUniformBuffers, GlobalDescriptorSets );
}

void LumenEngine::VulkanRHI::FVulkanMemory::DestroyDescriptors ( VkDevice InDevice ) noexcept
{
    if ( DescriptorPool != VK_NULL_HANDLE )
    {
        vkDestroyDescriptorPool( InDevice, DescriptorPool, nullptr );
        DescriptorPool = VK_NULL_HANDLE;
    }

    if ( GlobalSetLayout != VK_NULL_HANDLE )
    {
        vkDestroyDescriptorSetLayout( InDevice, GlobalSetLayout, nullptr );
        GlobalSetLayout = VK_NULL_HANDLE;
    }

    for ( UInt32 Index = 0; Index < MaxFramesInFlight; ++Index )
    {
        if ( GlobalUniformBuffers[Index].Buffer != VK_NULL_HANDLE )
        {
            vmaDestroyBuffer( Allocator, GlobalUniformBuffers[Index].Buffer, GlobalUniformBuffers[Index].Allocation );
            GlobalUniformBuffers[Index].Buffer = VK_NULL_HANDLE;
        }
    }
}

void LumenEngine::VulkanRHI::FVulkanMemory::DestroyVMA () noexcept
{
    if ( Allocator != VK_NULL_HANDLE )
    {
        vmaDestroyAllocator( Allocator );
        Allocator = VK_NULL_HANDLE;
    }
}

void LumenEngine::VulkanRHI::FVulkanMemory::UpdateGlobalUniformData ( UInt32 InFrameIndex, const RHI::FGlobalUniformData &InUniforms ) noexcept
{
    std::memcpy( GlobalUniformBuffers[InFrameIndex].AllocationInfo.pMappedData, &InUniforms, sizeof( RHI::FGlobalUniformData ) );
}

VmaAllocator LumenEngine::VulkanRHI::FVulkanMemory::GetAllocator () const noexcept
{
    return Allocator;
}

VkDescriptorSetLayout LumenEngine::VulkanRHI::FVulkanMemory::GetGlobalSetLayout () const noexcept
{
    return GlobalSetLayout;
}

VkDescriptorSet LumenEngine::VulkanRHI::FVulkanMemory::GetGlobalDescriptorSet ( UInt32 InFrameIndex ) const noexcept
{
    return GlobalDescriptorSets[InFrameIndex];
}
