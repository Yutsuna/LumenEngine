/**
 * @file GPUGlobalUniforms.hpp
 * @brief Extended per-frame uniform buffer that includes frustum planes for GPU culling.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Maths/Matrix.hpp"

#include "Vulkan/GPUDriven/GPUSceneTypes.hpp"

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @struct FGPUGlobalUniforms
     * @brief Full per-frame uniform block uploaded to set=0 binding=0.
     *
     *  Offset  Size   Field
     *  ------  ----   -------------------------------------------------------
     *       0    64   ViewProjectionMatrix  (mat4, 4×vec4)
     *      64     4   TimeSeconds
     *      68     4   DeltaTime
     *      72     8   Pad0
     *      80    96   FrustumPlanes[6]      (6 × vec4 = 96 bytes)
     *     176    16   Pad1                 (round up to 192 bytes total)
     *  Total: 192 bytes
     */
    struct alignas( 16 ) FGPUGlobalUniforms final
    {
        /** Combined View-Projection matrix (column-major). */
        Maths::FMatrix4x4f ViewProjectionMatrix = Maths::FMatrix4x4f::Identity();

        /** Seconds elapsed since engine start. */
        Float32 TimeSeconds = 0.0F;

        /** Delta-time for the current frame in seconds. */
        Float32 DeltaTime = 0.0F;

        /** Explicit padding to reach a 16-byte boundary before the planes. */
        Float32 Pad0[2] = {};

        /**
         * Six world-space frustum planes, each stored as (nx, ny, nz, d).
         * A point P is inside the frustum if dot(plane.xyz, P) + plane.w >= 0
         * for all six planes.
         */
        FGPUFrustumData Frustum;

        /** Pad to a multiple of 16 bytes for std140 compliance. */
        Float32 Pad1[4] = {};

        /** Factory: build from the camera VP matrix and engine time. */
        static FGPUGlobalUniforms Build ( const Maths::FMatrix4x4f &InVP, Float32 InTimeSeconds, Float32 InDeltaTime ) noexcept;
    };

    static_assert( sizeof( FGPUGlobalUniforms ) == 192, "FGPUGlobalUniforms must be 192 bytes for correct std140 UBO layout." );

} // namespace VulkanRHI

} // namespace LumenEngine
