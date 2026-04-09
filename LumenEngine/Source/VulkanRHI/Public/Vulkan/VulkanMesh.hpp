/**
 * @file VulkanMesh.hpp
 * @brief Declaration of the FVulkanMesh class for handling vertex and index buffers.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Container/Vector.hpp"
#include "Maths/Vertex.hpp"
#include "Vulkan/VulkanBuffer.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FVulkanMesh
     * @brief Encapsulates vertex and index buffer management for a mesh in Vulkan.
     */
    class LUMEN_ENGINE_API FVulkanMesh final
    {
    public:

        FVulkanMesh () noexcept  = default;
        ~FVulkanMesh () noexcept = default;

    public:

        /**
         * @brief Initializes the mesh by creating vertex and index buffers.
         * @param InAllocator The VMA allocator to use for buffer creation.
         * @param InVertices The vertex data for the mesh.
         * @param InIndices The index data for the mesh.
         */
        void Initialize ( VmaAllocator InAllocator, const TVector<Maths::FVertex> &InVertices, const TVector<UInt32> &InIndices );
        
        /**
         * @brief Cleans up the mesh by destroying vertex and index buffers.
         * @param InAllocator The VMA allocator used for buffer destruction.
         */
        void Cleanup ( VmaAllocator InAllocator ) noexcept;

        /**
         * @brief Binds the vertex and index buffers and issues a draw call.
         * @param InCommandBuffer The command buffer to record commands into.
         */
        void BindAndDraw ( VkCommandBuffer InCommandBuffer ) const noexcept;

    private:

        FVulkanBuffer VertexBuffer;
        FVulkanBuffer IndexBuffer;
        UInt32 IndexCount = 0;
    };

} // namespace VulkanRHI

} // namespace LumenEngine