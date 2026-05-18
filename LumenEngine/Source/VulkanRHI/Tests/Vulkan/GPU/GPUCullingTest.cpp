/**
 * @file GPUCullingTest.cpp
 * @brief Functional tests for GPU-driven culling data types.
 */

#include "Vulkan/GPUDriven/GPUGlobalUniforms.hpp"
#include "Vulkan/GPUDriven/GPUSceneTypes.hpp"

#include "Maths/Macros.hpp"
#include "Maths/Matrix.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace LumenEngine
{

namespace
{

    Bool TestAABBFrustumCPU ( const Maths::FVec3f &AABBMin, const Maths::FVec3f &AABBMax, const VulkanRHI::FGPUFrustumData &Frustum ) noexcept
    {
        for ( Int32 Index = 0; Index < 6; ++Index )
        {
            const Maths::FVec4f &Plane = Frustum.Planes[Index];
            const Maths::FVec3f PVertex{
                ( Plane.X >= 0.F ) ? AABBMax.X : AABBMin.X,
                ( Plane.Y >= 0.F ) ? AABBMax.Y : AABBMin.Y,
                ( Plane.Z >= 0.F ) ? AABBMax.Z : AABBMin.Z,
            };

            const Float32 Dist = ( Plane.X * PVertex.X ) + ( Plane.Y * PVertex.Y ) + ( Plane.Z * PVertex.Z ) + Plane.W;

            if ( Dist < 0.F )
            {
                return false;
            }
        }
        return true;
    }

    Maths::FMatrix4x4f BuildTestVP ()
    {
        const Maths::FMatrix4x4f View = Maths::FMatrix4x4f::LookAt( Maths::FVec3f{ 0.F, 0.F, 5.F }, Maths::FVec3f{ 0.F, 0.F, 0.F }, Maths::FVec3f{ 0.F, -1.F, 0.F } );

        const Float32 FovRad          = 90.F * static_cast<Float32>( DegToRad );
        const Maths::FMatrix4x4f Proj = Maths::FMatrix4x4f::Perspective( FovRad, 1.F, 0.1F, 100.F );

        return Proj * View;
    }

} // namespace

TEST( GPUDrivenCulling, InstanceDataLayout )
{
    EXPECT_EQ( sizeof( VulkanRHI::FGPUInstanceData ), 128ULL ) << "FGPUInstanceData must be exactly 128 bytes for correct "
                                                                  "std430 array stride.";

    EXPECT_EQ( alignof( VulkanRHI::FGPUInstanceData ), 16ULL ) << "FGPUInstanceData must be 16-byte aligned (std430 requirement).";

    EXPECT_EQ( sizeof( VulkanRHI::FGPUCullPushConstants ), 16ULL ) << "FGPUCullPushConstants must be 16 bytes.";
}

TEST( GPUDrivenCulling, GlobalUniformsLayout )
{
    EXPECT_EQ( sizeof( VulkanRHI::FGPUGlobalUniforms ), 192ULL ) << "FGPUGlobalUniforms must be 192 bytes for correct std140 UBO layout.";

    EXPECT_EQ( alignof( VulkanRHI::FGPUGlobalUniforms ), 16ULL );
}

TEST( GPUDrivenCulling, FrustumExtractionNotAllZero )
{
    const Maths::FMatrix4x4f VP              = BuildTestVP();
    const VulkanRHI::FGPUFrustumData Frustum = VulkanRHI::FGPUFrustumData::ExtractFromViewProjection( VP );

    for ( Int32 Index = 0; Index < 6; ++Index )
    {
        const Maths::FVec4f &P  = Frustum.Planes[Index];
        const Float32 NormalLen = std::sqrt( P.X * P.X + P.Y * P.Y + P.Z * P.Z );
        EXPECT_GT( NormalLen, 0.9F ) << "Plane " << Index << " has near-zero normal after normalisation.";
        EXPECT_LT( NormalLen, 1.1F ) << "Plane " << Index << " normal is not unit-length.";
    }
}

TEST( GPUDrivenCulling, OriginInsideFrustum )
{
    const Maths::FMatrix4x4f VP              = BuildTestVP();
    const VulkanRHI::FGPUFrustumData Frustum = VulkanRHI::FGPUFrustumData::ExtractFromViewProjection( VP );

    const Bool bVisible = TestAABBFrustumCPU( Maths::FVec3f{ -0.1F, -0.1F, -0.1F }, Maths::FVec3f{ 0.1F, 0.1F, 0.1F }, Frustum );

    EXPECT_TRUE( bVisible ) << "A box at the origin should be visible from z=5 looking at origin.";
}

TEST( GPUDrivenCulling, BoxBehindCameraIsCulled )
{
    const Maths::FMatrix4x4f VP              = BuildTestVP();
    const VulkanRHI::FGPUFrustumData Frustum = VulkanRHI::FGPUFrustumData::ExtractFromViewProjection( VP );

    const Bool bVisible = TestAABBFrustumCPU( Maths::FVec3f{ -0.5F, -0.5F, 45.F }, Maths::FVec3f{ 0.5F, 0.5F, 50.F }, Frustum );

    EXPECT_FALSE( bVisible ) << "A box behind the camera (z=50) should be culled.";
}

TEST( GPUDrivenCulling, BoxBeyondFarPlaneIsCulled )
{
    const Maths::FMatrix4x4f VP              = BuildTestVP();
    const VulkanRHI::FGPUFrustumData Frustum = VulkanRHI::FGPUFrustumData::ExtractFromViewProjection( VP );

    const Bool bVisible = TestAABBFrustumCPU( Maths::FVec3f{ -0.5F, -0.5F, -150.F }, Maths::FVec3f{ 0.5F, 0.5F, -145.F }, Frustum );

    EXPECT_FALSE( bVisible ) << "A box beyond the far plane should be culled.";
}

TEST( GPUDrivenCulling, BoxJustInsideFarPlane )
{
    const Maths::FMatrix4x4f VP              = BuildTestVP();
    const VulkanRHI::FGPUFrustumData Frustum = VulkanRHI::FGPUFrustumData::ExtractFromViewProjection( VP );

    const Bool bVisible = TestAABBFrustumCPU( Maths::FVec3f{ -0.1F, -0.1F, -94.F }, Maths::FVec3f{ 0.1F, 0.1F, -90.F }, Frustum );

    EXPECT_TRUE( bVisible ) << "A box just inside the far plane should be visible.";
}

TEST( GPUDrivenCulling, GPUGlobalUniformsBuildFactory )
{
    const Maths::FMatrix4x4f VP = BuildTestVP();

    const VulkanRHI::FGPUGlobalUniforms Uniforms = VulkanRHI::FGPUGlobalUniforms::Build( VP, 1.0F, 0.016F );

    EXPECT_EQ( Uniforms.ViewProjectionMatrix, VP );
    EXPECT_FLOAT_EQ( Uniforms.TimeSeconds, 1.0F );
    EXPECT_FLOAT_EQ( Uniforms.DeltaTime, 0.016F );

    Bool bAnyNonZero = false;
    for ( Int32 Index = 0; Index < 6; ++Index )
    {
        const Maths::FVec4f &P = Uniforms.Frustum.Planes[Index];
        if ( std::abs( P.X ) + std::abs( P.Y ) + std::abs( P.Z ) > 0.01F )
        {
            bAnyNonZero = true;
            break;
        }
    }
    EXPECT_TRUE( bAnyNonZero ) << "FGPUGlobalUniforms::Build() should populate frustum planes.";
}

} // namespace LumenEngine