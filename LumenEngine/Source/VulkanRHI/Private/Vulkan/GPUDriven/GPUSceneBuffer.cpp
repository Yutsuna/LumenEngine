/**
 * @file GPUSceneBuffer.cpp
 * @brief Implementation of the GPU scene SSBO upload system.
 */

#include "Vulkan/GPUDriven/GPUSceneBuffer.hpp"

#include "RHI/RHITypes.hpp"
#include "RHI/ResourceRegistry.hpp"

#include "Vulkan/GPUDriven/GPUSceneTypes.hpp"
#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanDescriptorWriter.hpp"
#include "Vulkan/VulkanMesh.hpp"
#include "Vulkan/VulkanPipeline.hpp"

#include "Logging/Logger.hpp"

#include <algorithm>

void LumenEngine::VulkanRHI::FGPUSceneBuffer::Initialize ( VmaAllocator InAllocator,
                                                           VkDevice InDevice,
                                                           VkDescriptorPool InDescPool,
                                                           VkDescriptorSetLayout InSceneSetLayout )
{
    Allocator = InAllocator;

    const VkDeviceSize BufferSize = MaxInstances * sizeof( FGPUInstanceData );

    VkBufferCreateInfo BufferInfo{};
    BufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size        = BufferSize;
    BufferInfo.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo AllocInfo{};
    AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    AllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    for ( UInt32 Index = 0U; Index < MaxFramesInFlight; ++Index )
    {
        LUMEN_VK_CHECK( vmaCreateBuffer( InAllocator, &BufferInfo, &AllocInfo, &SSBOs[Index].Buffer, &SSBOs[Index].Allocation, &SSBOs[Index].AllocationInfo ) );
    }

    TVector<VkDescriptorSetLayout> Layouts( MaxFramesInFlight, InSceneSetLayout );

    VkDescriptorSetAllocateInfo SetAllocInfo{};
    SetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    SetAllocInfo.descriptorPool     = InDescPool;
    SetAllocInfo.descriptorSetCount = MaxFramesInFlight;
    SetAllocInfo.pSetLayouts        = Layouts.data();

    LUMEN_VK_CHECK( vkAllocateDescriptorSets( InDevice, &SetAllocInfo, DescriptorSets ) );

    for ( UInt32 Index = 0U; Index < MaxFramesInFlight; ++Index )
    {
        FVulkanDescriptorWriter Writer;
        Writer.WriteBuffer( 0, SSBOs[Index].Buffer, BufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
        Writer.UpdateSet( InDevice, DescriptorSets[Index] );
    }

    LUMEN_LOG_INFO( LogVulkanRHI, "GPUSceneBuffer initialized: {} frames x {} bytes per frame ({} max instances).", MaxFramesInFlight, static_cast<UInt64>( BufferSize ),
                    MaxInstances );
}

void LumenEngine::VulkanRHI::FGPUSceneBuffer::Shutdown ( VmaAllocator InAllocator, VkDevice /*InDevice*/ ) noexcept
{
    for ( UInt32 Index = 0U; Index < MaxFramesInFlight; ++Index )
    {
        if ( SSBOs[Index].Buffer != VK_NULL_HANDLE )
        {
            vmaDestroyBuffer( InAllocator, SSBOs[Index].Buffer, SSBOs[Index].Allocation );
            SSBOs[Index].Buffer     = VK_NULL_HANDLE;
            SSBOs[Index].Allocation = VK_NULL_HANDLE;
        }
    }
}

LumenEngine::UInt32 LumenEngine::VulkanRHI::FGPUSceneBuffer::Upload ( const RHI::FSceneSnapshot &InSnapshot,
                                                                      const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &MeshRegistry,
                                                                      const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &PipelineRegistry,
                                                                      UInt32 InFrameIndex )
{
    static_cast<void>( PipelineRegistry );

    const USize EntityCount = std::min( { InSnapshot.Transforms.size(), InSnapshot.Meshes.size(), InSnapshot.Shaders.size() } );

    if ( EntityCount == 0U )
    {
        LastInstanceCount = 0U;
        return 0U;
    }

    const USize WriteCount = std::min( EntityCount, MaxInstances );
    FGPUInstanceData *Dest = static_cast<FGPUInstanceData *>( SSBOs[InFrameIndex].AllocationInfo.pMappedData );
    UInt32 ValidCount      = 0U;

    for ( USize EntityIndex = 0U; EntityIndex < WriteCount; ++EntityIndex )
    {
        const RHI::FMeshHandle MeshHandle       = InSnapshot.Meshes[EntityIndex];
        const RHI::FPipelineHandle ShaderHandle = InSnapshot.Shaders[EntityIndex];

        if ( not MeshHandle.IsValid() or not ShaderHandle.IsValid() )
        {
            continue;
        }

        const FVulkanMesh *Mesh = MeshRegistry.Get( MeshHandle );

        if ( Mesh == nullptr )
        {
            continue;
        }

        FGPUInstanceData &Instance = Dest[ValidCount];

        Instance.Transform = InSnapshot.Transforms[EntityIndex];

        /** INFO: Extract Object-space AABB from the Mesh resource */
        Instance.AABBMin = Mesh->GetAABBMin();
        Instance.AABBMax = Mesh->GetAABBMax();

        Instance.MeshHandleID   = MeshHandle.ID;
        Instance.ShaderHandleID = ShaderHandle.ID;

        Instance.IndexCount   = Mesh->GetIndexCount();
        Instance.FirstIndex   = Mesh->GetFirstIndex();
        Instance.VertexOffset = Mesh->GetVertexOffset();
        Instance.Pad0         = 0U;

        Instance.VertexBufferAddr = Mesh->GetVertexBufferAddress();
        Instance.IndexBufferAddr  = Mesh->GetIndexBufferAddress();

        ++ValidCount;
    }

    const VkDeviceSize FlushedSize = static_cast<VkDeviceSize>( ValidCount * sizeof( FGPUInstanceData ) );

    if ( FlushedSize > 0ULL )
    {
        LUMEN_VK_CHECK( vmaFlushAllocation( Allocator, SSBOs[InFrameIndex].Allocation, 0, FlushedSize ) );
    }

    LastInstanceCount = ValidCount;
    return ValidCount;
}

VkBuffer LumenEngine::VulkanRHI::FGPUSceneBuffer::GetBuffer ( UInt32 InFrameIndex ) const noexcept
{
    return SSBOs[InFrameIndex].Buffer;
}

VkDescriptorSet LumenEngine::VulkanRHI::FGPUSceneBuffer::GetDescriptorSet ( UInt32 InFrameIndex ) const noexcept
{
    return DescriptorSets[InFrameIndex];
}

LumenEngine::UInt32 LumenEngine::VulkanRHI::FGPUSceneBuffer::GetLastInstanceCount () const noexcept
{
    return LastInstanceCount;
}
