/**
 * @file VulkanMemory.cpp
 * @brief Implementation of Vulkan memory and global descriptors manager.
 */

#include "Vulkan/VulkanMemory.hpp"

#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanDescriptorWriter.hpp"
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace
{

[[nodiscard]] VkDescriptorSetLayout CreateGlobalSetLayout ( VkDevice InDevice )
{
    const VkDescriptorSetLayoutBinding Binding{
        .binding            = 0U,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1U,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo LayoutInfo{
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = nullptr,
        .flags        = 0U,
        .bindingCount = 1U,
        .pBindings    = &Binding,
    };

    VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
    LUMEN_VK_CHECK( vkCreateDescriptorSetLayout( InDevice, &LayoutInfo, nullptr, &Layout ) );
    return Layout;
}

[[nodiscard]] VkDescriptorSetLayout CreateSceneSetLayout ( VkDevice InDevice )
{
    const VkDescriptorSetLayoutBinding Binding{
        .binding            = 0U,
        .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount    = 1U,
        .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo LayoutInfo{
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = nullptr,
        .flags        = 0U,
        .bindingCount = 1U,
        .pBindings    = &Binding,
    };

    VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
    LUMEN_VK_CHECK( vkCreateDescriptorSetLayout( InDevice, &LayoutInfo, nullptr, &Layout ) );
    return Layout;
}

[[nodiscard]] VkDescriptorSetLayout CreateCullSetLayout ( VkDevice InDevice )
{
    const VkDescriptorSetLayoutBinding Bindings[2] = {
        {
            .binding            = 0U,
            .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount    = 1U,
            .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding            = 1U,
            .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount    = 1U,
            .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr,
        },
    };

    VkDescriptorSetLayoutCreateInfo LayoutInfo{
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = nullptr,
        .flags        = 0U,
        .bindingCount = 2U,
        .pBindings    = Bindings,
    };

    VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
    LUMEN_VK_CHECK( vkCreateDescriptorSetLayout( InDevice, &LayoutInfo, nullptr, &Layout ) );
    return Layout;
}

void CreateDescriptorPool ( VkDevice InDevice, LumenEngine::UInt32 InMaxFramesInFlight, VkDescriptorPool &OutPool )
{
    if ( InMaxFramesInFlight == 0U )
    {
        OutPool = VK_NULL_HANDLE;
        return;
    }

    const VkDescriptorPoolSize PoolSizes[] = {
        { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = InMaxFramesInFlight },
        { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = InMaxFramesInFlight * 4U },
    };

    VkDescriptorPoolCreateInfo PoolInfo{
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = InMaxFramesInFlight * 4U,
        .poolSizeCount = 2U,
        .pPoolSizes    = PoolSizes,
    };

    LUMEN_VK_CHECK( vkCreateDescriptorPool( InDevice, &PoolInfo, nullptr, &OutPool ) );
}

[[nodiscard]] inline VkBufferCreateInfo CreateBufferInfo ( LumenEngine::USize InSize ) noexcept
{
    return VkBufferCreateInfo{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0U,
        .size                  = InSize,
        .usage                 = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0U,
        .pQueueFamilyIndices   = nullptr,
    };
}

[[nodiscard]] inline VmaAllocatorCreateInfo
CreateVmaAllocatorInfo ( VkInstance InInstance, VkPhysicalDevice InPhysicalDevice, VkDevice InDevice, VmaVulkanFunctions *InFunctions ) noexcept
{
    VmaAllocatorCreateInfo VmaAllocCreateInfo{};
    VmaAllocCreateInfo.flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    VmaAllocCreateInfo.physicalDevice   = InPhysicalDevice;
    VmaAllocCreateInfo.device           = InDevice;
    VmaAllocCreateInfo.pVulkanFunctions = InFunctions;
    VmaAllocCreateInfo.instance         = InInstance;
    VmaAllocCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    return VmaAllocCreateInfo;
}

[[nodiscard]] inline VmaAllocationCreateInfo CreateVmaAllocationInfo () noexcept
{
    VmaAllocationCreateInfo AllocInfo{};

    AllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    return AllocInfo;
}

} // namespace

void LumenEngine::VulkanRHI::FVulkanMemory::Initialize ( VkInstance InInstance, VkPhysicalDevice InPhysicalDevice, VkDevice InDevice, const FDescriptorConfig &InConfig )
{
    NumFramesInFlight = InConfig.MaxFramesInFlight;
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
    VmaVulkanFunctions VulkanFunctions{};
    VulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    VulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo AllocatorInfo = CreateVmaAllocatorInfo( InInstance, InPhysicalDevice, InDevice, &VulkanFunctions );
    LUMEN_VK_CHECK( vmaCreateAllocator( &AllocatorInfo, &Allocator ) );
}

void LumenEngine::VulkanRHI::FVulkanMemory::InitializeDescriptors ( VkDevice InDevice )
{
    GlobalSetLayout = CreateGlobalSetLayout( InDevice );
    SceneSetLayout  = CreateSceneSetLayout( InDevice );
    CullSetLayout   = CreateCullSetLayout( InDevice );

    CreateDescriptorPool( InDevice, NumFramesInFlight, DescriptorPool );

    if ( NumFramesInFlight == 0U )
    {
        return;
    }

    GlobalUniformBuffers.resize( NumFramesInFlight );
    GlobalDescriptorSets.resize( NumFramesInFlight );

    VkBufferCreateInfo BufferInfo     = CreateBufferInfo( sizeof( FGPUGlobalUniforms ) );
    VmaAllocationCreateInfo AllocInfo = CreateVmaAllocationInfo();

    for ( UInt32 Index = 0U; Index < NumFramesInFlight; ++Index )
    {
        LUMEN_VK_CHECK( vmaCreateBuffer( Allocator, &BufferInfo, &AllocInfo, &GlobalUniformBuffers[Index].Buffer, &GlobalUniformBuffers[Index].Allocation,
                                         &GlobalUniformBuffers[Index].AllocationInfo ) );

        VkDescriptorSetAllocateInfo SetAllocInfo{ .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                  .pNext              = nullptr,
                                                  .descriptorPool     = DescriptorPool,
                                                  .descriptorSetCount = 1U,
                                                  .pSetLayouts        = &GlobalSetLayout };

        LUMEN_VK_CHECK( vkAllocateDescriptorSets( InDevice, &SetAllocInfo, &GlobalDescriptorSets[Index] ) );

        FVulkanDescriptorWriter Writer;
        Writer.WriteBuffer( 0U, GlobalUniformBuffers[Index].Buffer, sizeof( FGPUGlobalUniforms ), 0U, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER );
        Writer.UpdateSet( InDevice, GlobalDescriptorSets[Index] );
    }
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
    if ( SceneSetLayout != VK_NULL_HANDLE )
    {
        vkDestroyDescriptorSetLayout( InDevice, SceneSetLayout, nullptr );
        SceneSetLayout = VK_NULL_HANDLE;
    }
    if ( CullSetLayout != VK_NULL_HANDLE )
    {
        vkDestroyDescriptorSetLayout( InDevice, CullSetLayout, nullptr );
        CullSetLayout = VK_NULL_HANDLE;
    }

    for ( UInt32 Index = 0U; Index < NumFramesInFlight; ++Index )
    {
        if ( GlobalUniformBuffers[Index].Buffer != VK_NULL_HANDLE )
        {
            vmaDestroyBuffer( Allocator, GlobalUniformBuffers[Index].Buffer, GlobalUniformBuffers[Index].Allocation );
        }
    }

    GlobalUniformBuffers.clear();
    GlobalDescriptorSets.clear();
    NumFramesInFlight = 0U;
}

void LumenEngine::VulkanRHI::FVulkanMemory::DestroyVMA () noexcept
{
    if ( Allocator != VK_NULL_HANDLE )
    {
        vmaDestroyAllocator( Allocator );
        Allocator = VK_NULL_HANDLE;
    }
}

void LumenEngine::VulkanRHI::FVulkanMemory::UpdateGlobalUniformData ( UInt32 InFrameIndex, const FGPUGlobalUniforms &InUniforms ) noexcept
{
    if ( InFrameIndex >= NumFramesInFlight or GlobalUniformBuffers[InFrameIndex].Buffer == VK_NULL_HANDLE )
    {
        return;
    }
    std::memcpy( GlobalUniformBuffers[InFrameIndex].AllocationInfo.pMappedData, &InUniforms, sizeof( FGPUGlobalUniforms ) );
    vmaFlushAllocation( Allocator, GlobalUniformBuffers[InFrameIndex].Allocation, 0, sizeof( FGPUGlobalUniforms ) );
}

void LumenEngine::VulkanRHI::FVulkanMemory::UpdateGlobalUniformData ( UInt32 InFrameIndex, const RHI::FGlobalUniformData &InUniforms ) noexcept
{
    FGPUGlobalUniforms GPUData = FGPUGlobalUniforms::Build( InUniforms.ViewProjectionMatrix, InUniforms.TimeSeconds, InUniforms.DeltaTime );

    UpdateGlobalUniformData( InFrameIndex, GPUData );
}

VmaAllocator LumenEngine::VulkanRHI::FVulkanMemory::GetAllocator () const noexcept
{
    return Allocator;
}

VkDescriptorPool LumenEngine::VulkanRHI::FVulkanMemory::GetDescriptorPool () const noexcept
{
    return DescriptorPool;
}

LumenEngine::UInt32 LumenEngine::VulkanRHI::FVulkanMemory::GetNumFramesInFlight () const noexcept
{
    return NumFramesInFlight;
}

VkDescriptorSetLayout LumenEngine::VulkanRHI::FVulkanMemory::GetGlobalSetLayout () const noexcept
{
    return GlobalSetLayout;
}

VkDescriptorSetLayout LumenEngine::VulkanRHI::FVulkanMemory::GetSceneSetLayout () const noexcept
{
    return SceneSetLayout;
}

VkDescriptorSetLayout LumenEngine::VulkanRHI::FVulkanMemory::GetCullSetLayout () const noexcept
{
    return CullSetLayout;
}

VkDescriptorSet LumenEngine::VulkanRHI::FVulkanMemory::GetGlobalDescriptorSet ( UInt32 InFrameIndex ) const noexcept
{
    if ( InFrameIndex >= NumFramesInFlight )
    {
        return VK_NULL_HANDLE;
    }
    return GlobalDescriptorSets[InFrameIndex];
}
