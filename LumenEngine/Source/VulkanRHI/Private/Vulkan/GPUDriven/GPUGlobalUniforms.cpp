b/**
 * @file GPUGlobalUniforms.cpp
 * @brief Implementation of FGPUGlobalUniforms factory.
 */

#include "Vulkan/GPUDriven/GPUGlobalUniforms.hpp"

namespace LumenEngine
{

namespace VulkanRHI
{

    FGPUGlobalUniforms FGPUGlobalUniforms::Build ( const Maths::FMatrix4x4f &InVP, Float32 InTimeSeconds, Float32 InDeltaTime ) noexcept
    {
        FGPUGlobalUniforms Out;
        Out.ViewProjectionMatrix = InVP;
        Out.TimeSeconds          = InTimeSeconds;
        Out.DeltaTime            = InDeltaTime;
        Out.Frustum              = FGPUFrustumData::ExtractFromViewProjection( InVP );
        return Out;
    }

} // namespace VulkanRHI

} // namespace LumenEngine
