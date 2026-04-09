/**
 * @file VulkanMesh.cpp
 * @brief Implementation of the FVulkanMesh class.
 */

#include "Vulkan/VulkanMesh.hpp"
#include "Vulkan/VulkanCore.hpp"

#include <cstring>

namespace
{

LumenEngine::VulkanRHI::FVulkanBuffer CreateBufferInternal ( VmaAllocator InAllocator, LumenEngine::USize InSize, VkBufferUsageFlags InUsage, VmaMemoryUsage InMemoryUsage ) 
{
    LumenEngine::VulkanRHI::FVulkanBuffer Buffer;

    VkBufferCreateInfo BufferInfo{};
    BufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size        = InSize;
    BufferInfo.usage       = InUsage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo AllocInfo{};
    AllocInfo.usage = InMemoryUsage;
    AllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    LUMEN_VK_CHECK( vmaCreateBuffer( InAllocator, &BufferInfo, &AllocInfo, &Buffer.Buffer, &Buffer.Allocation, &Buffer.AllocationInfo ) );

    return Buffer;
}

}

void LumenEngine::VulkanRHI::FVulkanMesh::Initialize ( VmaAllocator InAllocator,
                                                       const LumenEngine::TVector<LumenEngine::Maths::FVertex> &InVertices,
                                                       const LumenEngine::TVector<LumenEngine::UInt32> &InIndices )
{
    IndexCount = static_cast<LumenEngine::UInt32>( InIndices.size() );

    const LumenEngine::USize VertexSize = InVertices.size() * sizeof( LumenEngine::Maths::FVertex );
    VertexBuffer                        = CreateBufferInternal( InAllocator, VertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO );
    std::memcpy( VertexBuffer.AllocationInfo.pMappedData, InVertices.data(), VertexSize );

    const LumenEngine::USize IndexSize = InIndices.size() * sizeof( LumenEngine::UInt32 );
    IndexBuffer                        = CreateBufferInternal( InAllocator, IndexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO );
    std::memcpy( IndexBuffer.AllocationInfo.pMappedData, InIndices.data(), IndexSize );
}

void LumenEngine::VulkanRHI::FVulkanMesh::Cleanup ( VmaAllocator InAllocator ) noexcept
{
    if ( VertexBuffer.Buffer != VK_NULL_HANDLE )
    {
        vmaDestroyBuffer( InAllocator, VertexBuffer.Buffer, VertexBuffer.Allocation );
        VertexBuffer.Buffer = VK_NULL_HANDLE;
    }

    if ( IndexBuffer.Buffer != VK_NULL_HANDLE )
    {
        vmaDestroyBuffer( InAllocator, IndexBuffer.Buffer, IndexBuffer.Allocation );
        IndexBuffer.Buffer = VK_NULL_HANDLE;
    }
}

void LumenEngine::VulkanRHI::FVulkanMesh::BindAndDraw ( VkCommandBuffer InCommandBuffer ) const noexcept
{
    VkDeviceSize Offset = 0;

    vkCmdBindVertexBuffers( InCommandBuffer, 0, 1, &VertexBuffer.Buffer, &Offset );
    vkCmdBindIndexBuffer( InCommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32 );
    vkCmdDrawIndexed( InCommandBuffer, IndexCount, 1, 0, 0, 0 );
}
