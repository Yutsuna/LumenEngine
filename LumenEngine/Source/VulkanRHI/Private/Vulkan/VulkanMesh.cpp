/**
 * @file VulkanMesh.cpp
 * @brief Updated implementation of FVulkanMesh with BDA and AABB support.
 */

#include "Vulkan/VulkanMesh.hpp"
#include "Vulkan/VulkanCore.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

namespace
{

/**
 * @brief Creates a CPU-mappable buffer of the given size and usage.
 * Adds VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT to every buffer so that
 * BDA can be queried later.
 */
LumenEngine::VulkanRHI::FVulkanBuffer CreateStagedBuffer ( VmaAllocator InAllocator, LumenEngine::USize InSize, VkBufferUsageFlags InUsage )
{
    LumenEngine::VulkanRHI::FVulkanBuffer Buffer;

    VkBufferCreateInfo BufferInfo{};
    BufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size        = static_cast<VkDeviceSize>( InSize );
    BufferInfo.usage       = InUsage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo AllocInfo{};
    AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    AllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    LUMEN_VK_CHECK( vmaCreateBuffer( InAllocator, &BufferInfo, &AllocInfo, &Buffer.Buffer, &Buffer.Allocation, &Buffer.AllocationInfo ) );

    return Buffer;
}

} // namespace

void LumenEngine::VulkanRHI::FVulkanMesh::Initialize ( VmaAllocator InAllocator,
                                                       VkDevice InDevice,
                                                       const TVector<Maths::FVertex> &InVertices,
                                                       const TVector<UInt32> &InIndices )
{
    IndexCount   = static_cast<UInt32>( InIndices.size() );
    FirstIndex   = 0U;
    VertexOffset = 0;

    /** Vertex buffer */
    const USize VertexSize = InVertices.size() * sizeof( Maths::FVertex );
    VertexBuffer           = CreateStagedBuffer( InAllocator, VertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );

    std::memcpy( VertexBuffer.AllocationInfo.pMappedData, InVertices.data(), VertexSize );

    /** Index buffer */
    const USize IndexSize = InIndices.size() * sizeof( UInt32 );
    IndexBuffer           = CreateStagedBuffer( InAllocator, IndexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );

    std::memcpy( IndexBuffer.AllocationInfo.pMappedData, InIndices.data(), IndexSize );

    /** AABB */
    ComputeAABB( InVertices );

    /** BDA */
    QueryBDA( InDevice );
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

    VertexBDA = 0ULL;
    IndexBDA  = 0ULL;
}

void LumenEngine::VulkanRHI::FVulkanMesh::BindAndDraw ( VkCommandBuffer InCommandBuffer ) const noexcept
{
    VkDeviceSize Offset = 0;
    vkCmdBindVertexBuffers( InCommandBuffer, 0, 1, &VertexBuffer.Buffer, &Offset );
    vkCmdBindIndexBuffer( InCommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32 );
    vkCmdDrawIndexed( InCommandBuffer, IndexCount, 1, FirstIndex, VertexOffset, 0 );
}

LumenEngine::UInt32 LumenEngine::VulkanRHI::FVulkanMesh::GetIndexCount () const noexcept
{
    return IndexCount;
}

LumenEngine::UInt32 LumenEngine::VulkanRHI::FVulkanMesh::GetFirstIndex () const noexcept
{
    return FirstIndex;
}

LumenEngine::Int32 LumenEngine::VulkanRHI::FVulkanMesh::GetVertexOffset () const noexcept
{
    return VertexOffset;
}

LumenEngine::UInt64 LumenEngine::VulkanRHI::FVulkanMesh::GetVertexBufferAddress () const noexcept
{
    return VertexBDA;
}

LumenEngine::UInt64 LumenEngine::VulkanRHI::FVulkanMesh::GetIndexBufferAddress () const noexcept
{
    return IndexBDA;
}

const LumenEngine::Maths::FVec3f &LumenEngine::VulkanRHI::FVulkanMesh::GetAABBMin () const noexcept
{
    return AABBMin;
}

const LumenEngine::Maths::FVec3f &LumenEngine::VulkanRHI::FVulkanMesh::GetAABBMax () const noexcept
{
    return AABBMax;
}

void LumenEngine::VulkanRHI::FVulkanMesh::ComputeAABB ( const TVector<Maths::FVertex> &InVertices ) noexcept
{
    if ( InVertices.empty() )
    {
        return;
    }

    constexpr Float32 FMax = std::numeric_limits<Float32>::max();
    constexpr Float32 FMin = std::numeric_limits<Float32>::lowest();

    Maths::FVec3f Min{ FMax, FMax, FMax };
    Maths::FVec3f Max{ FMin, FMin, FMin };

    for ( const Maths::FVertex &V : InVertices )
    {
        for ( USize Axis = 0; Axis < 3; ++Axis )
        {
            const Float32 Coord = V.Position.Data[Axis];
            Min.Data[Axis]      = std::min( Coord, Min.Data[Axis] );
            Max.Data[Axis]      = std::max( Coord, Max.Data[Axis] );
        }
    }

    AABBMin = Min;
    AABBMax = Max;
}

void LumenEngine::VulkanRHI::FVulkanMesh::QueryBDA ( VkDevice InDevice ) noexcept
{
    if ( InDevice == VK_NULL_HANDLE )
    {
        return;
    }

    if ( VertexBuffer.Buffer != VK_NULL_HANDLE )
    {
        VkBufferDeviceAddressInfo AddrInfo{};
        AddrInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        AddrInfo.buffer = VertexBuffer.Buffer;
        VertexBDA       = static_cast<UInt64>( vkGetBufferDeviceAddress( InDevice, &AddrInfo ) );
    }

    if ( IndexBuffer.Buffer != VK_NULL_HANDLE )
    {
        VkBufferDeviceAddressInfo AddrInfo{};
        AddrInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        AddrInfo.buffer = IndexBuffer.Buffer;
        IndexBDA        = static_cast<UInt64>( vkGetBufferDeviceAddress( InDevice, &AddrInfo ) );
    }
}
