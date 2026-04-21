/**
 * @file VulkanMesh.hpp
 * @brief Declaration of the FVulkanMesh class for handling vertex and index buffers.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Container/Vector.hpp"
#include "Maths/Vec.hpp"
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
     *
     * NOTE: Buffers are allocated with SHADER_DEVICE_ADDRESS_BIT so that
     * FGPUSceneBuffer can embed their BDA directly in FGPUInstanceData,
     * enabling potential future mesh-shader or ray-tracing use.
     */
    class LUMEN_ENGINE_API FVulkanMesh final
    {
    public:

        FVulkanMesh () noexcept  = default;
        ~FVulkanMesh () noexcept = default;

    public:

        /**
         * @brief Initializes the mesh by creating vertex and index buffers.
         *
         * Also computes a tight object-space AABB from the vertex positions.
         *
         * @param InAllocator The VMA allocator to use for buffer creation.
         * @param InDevice    Logical device for BDA queries.
         * @param InVertices  The vertex data for the mesh.
         * @param InIndices   The index data for the mesh.
         */
        void Initialize ( VmaAllocator InAllocator, VkDevice InDevice, const TVector<Maths::FVertex> &InVertices, const TVector<UInt32> &InIndices );

        /**
         * @brief Cleans up the mesh by destroying vertex and index buffers.
         * @param InAllocator The VMA allocator used for buffer destruction.
         */
        void Cleanup ( VmaAllocator InAllocator ) noexcept;

        /**
         * @brief Binds the vertex and index buffers and issues a draw call.
         *
         * Used for the legacy (non-GPU-driven) path and for shadow maps.
         *
         * @param InCommandBuffer The command buffer to record commands into.
         */
        void BindAndDraw ( VkCommandBuffer InCommandBuffer ) const noexcept;

    public:

        /** @return Number of indices (= draw call IndexCount). */
        [[nodiscard]] UInt32 GetIndexCount () const noexcept;

        /** @return First index in the index buffer (always 0 for standalone meshes). */
        [[nodiscard]] UInt32 GetFirstIndex () const noexcept;

        /** @return Vertex offset applied to every index (always 0 for standalone meshes). */
        [[nodiscard]] Int32 GetVertexOffset () const noexcept;

        /** @return BDA of the vertex buffer, or 0 if BDA is unavailable. */
        [[nodiscard]] UInt64 GetVertexBufferAddress () const noexcept;

        /** @return BDA of the index buffer, or 0 if BDA is unavailable. */
        [[nodiscard]] UInt64 GetIndexBufferAddress () const noexcept;

        /** @return Object-space AABB minimum corner (computed from vertices). */
        [[nodiscard]] const Maths::FVec3f &GetAABBMin () const noexcept;

        /** @return Object-space AABB maximum corner (computed from vertices). */
        [[nodiscard]] const Maths::FVec3f &GetAABBMax () const noexcept;

    private:

        void ComputeAABB ( const TVector<Maths::FVertex> &InVertices ) noexcept;
        void QueryBDA ( VkDevice InDevice ) noexcept;

    private:

        FVulkanBuffer VertexBuffer;
        FVulkanBuffer IndexBuffer;

        UInt32 IndexCount  = 0U;
        UInt32 FirstIndex  = 0U;
        Int32 VertexOffset = 0;

        UInt64 VertexBDA = 0ULL;
        UInt64 IndexBDA  = 0ULL;

        Maths::FVec3f AABBMin{ -0.5F, -0.5F, -0.5F };
        Maths::FVec3f AABBMax{ 0.5F, 0.5F, 0.5F };
    };

} // namespace VulkanRHI

} // namespace LumenEngine
