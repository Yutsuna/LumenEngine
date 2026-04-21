/**
 * @file GPUSceneTypes.cpp
 * @brief Implementation of FGPUFrustumData::ExtractFromViewProjection.
 */

#include "Vulkan/GPUDriven/GPUSceneTypes.hpp"

LumenEngine::VulkanRHI::FGPUFrustumData LumenEngine::VulkanRHI::FGPUFrustumData::ExtractFromViewProjection ( const Maths::FMatrix4x4f &VP ) noexcept
{
    const Maths::FVec4f R0 = VP.GetRow( 0 );
    const Maths::FVec4f R1 = VP.GetRow( 1 );
    const Maths::FVec4f R2 = VP.GetRow( 2 );
    const Maths::FVec4f R3 = VP.GetRow( 3 );

    FGPUFrustumData Out;

    Out.Planes[0] = Maths::NormalizePlane( R3 + R0 ); //<< Left
    Out.Planes[1] = Maths::NormalizePlane( R3 - R0 ); //<< Right
    Out.Planes[2] = Maths::NormalizePlane( R3 + R1 ); //<< Bottom
    Out.Planes[3] = Maths::NormalizePlane( R3 - R1 ); //<< Top
    Out.Planes[4] = Maths::NormalizePlane( R3 + R2 ); //<< Near
    Out.Planes[5] = Maths::NormalizePlane( R3 - R2 ); //<< Far

    return Out;
}
