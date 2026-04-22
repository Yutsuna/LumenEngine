/**
 * @file GPUSceneTypes.hpp
 * @brief POD types describing per-instance data uploaded to the GPU scene buffer.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Maths/Matrix.hpp"
#include "Maths/Vec.hpp"

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @struct FGPUInstanceData
     * @brief Per-instance record written into the GPU scene SSBO.
     *
     * std430 layout (208 bytes, 16-byte aligned):
     *
     *  Offset  Size  Field
     *  ------  ----  -------------------------------------------
     *       0    64  Transform          (column-major FMatrix4x4f)
     *      64    12  AABBMin            (FVec3f)
     *      76     4  MeshHandleID       (uint32)
     *      80    12  AABBMax            (FVec3f)
     *      84     4  ShaderHandleID     (uint32 — pipeline handle)
     *      88     4  IndexCount         (uint32)
     *      92     4  FirstIndex         (uint32)
     *      96     4  VertexOffset       (int32)
     *     100     4  Pad0              (explicit)
     *     104     8  VertexBufferAddr   (VkDeviceAddress / uint64)
     *     112     8  IndexBufferAddr    (VkDeviceAddress / uint64)
     *     120     8  Pad1              (align struct to 16 bytes)
     *  Total: 128 bytes
     */
    struct alignas( 16 ) FGPUInstanceData final
    {
        /** World-space transform (column-major, same memory layout as FMatrix4x4f). */
        Maths::FMatrix4x4f Transform = Maths::FMatrix4x4f::Identity();

        /** AABB min corner in object space. */
        Maths::FVec3f AABBMin{ -0.5F, -0.5F, -0.5F };

        /** Packed mesh handle index (into VulkanRHI::MeshRegistry). */
        UInt32 MeshHandleID = UINT32_MAX;

        /** AABB max corner in object space. */
        Maths::FVec3f AABBMax{ 0.5F, 0.5F, 0.5F };

        /** Packed pipeline handle index (into VulkanRHI::PipelineRegistry). */
        UInt32 ShaderHandleID = UINT32_MAX;

        /** Number of indices in the draw call. */
        UInt32 IndexCount = 0U;

        /** First index offset in the index buffer. */
        UInt32 FirstIndex = 0U;

        /** Vertex offset applied to every index. */
        Int32 VertexOffset = 0;

        /** Explicit padding — keeps VertexBufferAddr 8-byte aligned. */
        UInt32 Pad0 = 0U;

        /**
         * BDA (Buffer Device Address) of the vertex buffer.
         * Zero when mesh is not BDA-capable; in that case the vertex buffer is
         * bound via the traditional vkCmdBindVertexBuffers path.
         */
        UInt64 VertexBufferAddr = 0ULL;

        /** BDA of the index buffer (zero when not BDA-capable). */
        UInt64 IndexBufferAddr = 0ULL;
    };

    static_assert( sizeof( FGPUInstanceData ) == 128, "FGPUInstanceData must be exactly 128 bytes for correct std430 array stride." );
    static_assert( alignof( FGPUInstanceData ) == 16, "FGPUInstanceData must be 16-byte aligned to satisfy std430 layout." );

    /**
     * @struct FGPUCullPushConstants
     * @brief Push-constant block fed to the culling compute shader.
     *
     * std430 layout (32 bytes):
     *
     *  Offset  Size  Field
     *  ------  ----  -------------------------------------------
     *       0     4  InstanceCount   (uint32)
     *       4     4  Pad            (explicit)
     *       8     8  Reserved       (reserved for future use)
     *      16    16  Frustum planes are passed via UBO, not here.
     *
     * Kept small; frustum data is in the global UBO (set=0, binding=0).
     */
    struct FGPUCullPushConstants final
    {
        /** Total number of instances in the GPU scene buffer this frame. */
        UInt32 InstanceCount = 0U;

        /** Whether to skip culling entirely (e.g. shadow pass). */
        UInt32 bDisableCulling = 0U;

        /** Reserved. */
        UInt64 Reserved = 0ULL;
    };

    static_assert( sizeof( FGPUCullPushConstants ) == 16, "FGPUCullPushConstants must be exactly 16 bytes." );

    /**
     * @struct FGPUFrustumData
     * @brief Six frustum planes in world space.
     *
     * Appended to the global UBO that already contains ViewProjection + Time.
     * Each plane is stored as (Normal.xyz, Distance) in vec4 form so that
     * the GLSL dot(plane.xyz, worldPos) + plane.w >= 0 test is branchless.
     *
     * Plane order follows Gribb-Hartmann extraction from the VP matrix:
     *   [0] Left, [1] Right, [2] Bottom, [3] Top, [4] Near, [5] Far
     */
    struct alignas( 16 ) FGPUFrustumData final
    {
        Maths::FVec4f Planes[6] = {};

        /** Filled in from FCamera each frame before BeginFrame(). */
        static FGPUFrustumData ExtractFromViewProjection ( const Maths::FMatrix4x4f &VP ) noexcept;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
