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
        { .binding = 0U, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1U, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 1U, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1U, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
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

void CreateDescriptorPool ( VkDevice InDevice, VkDescriptorPool &OutPool )
{
    const VkDescriptorPoolSize PoolSizes[] = {
        { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = LumenEngine::VulkanRHI::MaxFramesInFlight },
        { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = LumenEngine::VulkanRHI::MaxFramesInFlight * 4U },
    };

    VkDescriptorPoolCreateInfo PoolInfo{
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = LumenEngine::VulkanRHI::MaxFramesInFlight * 4U,
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

[[nodiscard]] inline VmaAllocationCreateInfo CreateBufferAllocInfo () noexcept
{
    VmaAllocationCreateInfo AllocInfo{};

    AllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    return AllocInfo;
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
    VmaVulkanFunctions VulkanFunctions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr   = vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo AllocatorInfo{
        .flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice   = InPhysicalDevice,
        .device           = InDevice,
        .pVulkanFunctions = &VulkanFunctions,
        .instance         = InInstance,
        .vulkanApiVersion = VK_API_VERSION_1_3,
    };

    LUMEN_VK_CHECK( vmaCreateAllocator( &AllocatorInfo, &Allocator ) );
}

void LumenEngine::VulkanRHI::FVulkanMemory::InitializeDescriptors ( VkDevice InDevice )
{
    GlobalSetLayout = CreateGlobalSetLayout( InDevice );
    SceneSetLayout  = CreateSceneSetLayout( InDevice );
    CullSetLayout   = CreateCullSetLayout( InDevice );

    CreateDescriptorPool( InDevice, DescriptorPool );

    VkBufferCreateInfo BufferInfo     = CreateBufferInfo( sizeof( FGPUGlobalUniforms ) );
    VmaAllocationCreateInfo AllocInfo = CreateBufferAllocInfo();

    for ( UInt32 Index = 0U; Index < MaxFramesInFlight; ++Index )
    {
        LUMEN_VK_CHECK( vmaCreateBuffer( Allocator, &BufferInfo, &AllocInfo, &GlobalUniformBuffers[Index].Buffer, &GlobalUniformBuffers[Index].Allocation,
                                         &GlobalUniformBuffers[Index].AllocationInfo ) );

        VkDescriptorSetAllocateInfo SetAlloc{
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = DescriptorPool,
            .descriptorSetCount = 1U,
            .pSetLayouts        = &GlobalSetLayout,
        };
        LUMEN_VK_CHECK( vkAllocateDescriptorSets( InDevice, &SetAlloc, &GlobalDescriptorSets[Index] ) );

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
    }
    if ( GlobalSetLayout != VK_NULL_HANDLE )
    {
        vkDestroyDescriptorSetLayout( InDevice, GlobalSetLayout, nullptr );
    }
    if ( SceneSetLayout != VK_NULL_HANDLE )
    {
        vkDestroyDescriptorSetLayout( InDevice, SceneSetLayout, nullptr );
    }
    if ( CullSetLayout != VK_NULL_HANDLE )
    {
        vkDestroyDescriptorSetLayout( InDevice, CullSetLayout, nullptr );
    }

    for ( UInt32 Index = 0U; Index < MaxFramesInFlight; ++Index )
    {
        if ( GlobalUniformBuffers[Index].Buffer != VK_NULL_HANDLE )
        {
            vmaDestroyBuffer( Allocator, GlobalUniformBuffers[Index].Buffer, GlobalUniformBuffers[Index].Allocation );
        }
    }
}

void LumenEngine::VulkanRHI::FVulkanMemory::DestroyVMA () noexcept
{
    if ( Allocator != VK_NULL_HANDLE )
    {
        vmaDestroyAllocator( Allocator );
    }
}

void LumenEngine::VulkanRHI::FVulkanMemory::UpdateGlobalUniformData ( UInt32 InFrameIndex, const FGPUGlobalUniforms &InUniforms ) noexcept
{
    std::memcpy( GlobalUniformBuffers[InFrameIndex].AllocationInfo.pMappedData, &InUniforms, sizeof( FGPUGlobalUniforms ) );
}

void LumenEngine::VulkanRHI::FVulkanMemory::UpdateGlobalUniformData ( UInt32 InFrameIndex, const RHI::FGlobalUniformData &InUniforms ) noexcept
{
    FGPUGlobalUniforms GPUData = FGPUGlobalUniforms::Build( InUniforms.ViewProjectionMatrix, 0.0F, 0.016F );

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
    return GlobalDescriptorSets[InFrameIndex];
}