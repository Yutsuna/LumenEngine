/**
 * @file Camera.cpp
 * @brief Implementation of the FCamera class for 3D and 2D camera handling
 */

#include "Maths/Camera.hpp"
#include "Maths/Macros.hpp"
#include "Maths/Matrix.hpp"

/**
 * Public Methods
 */

void LumenEngine::Maths::FCamera::Tick ( const Float64 InDeltaTime ) noexcept
{
    ( void )InDeltaTime;
    if ( DirtyFlags == ECameraDirtyFlags::None ) [[likely]]
    {
        return;
    }

    UpdateMatrix();
}

/**
 * Private Methods
 */

void LumenEngine::Maths::FCamera::UpdateMatrix () noexcept
{
    if ( Bool( DirtyFlags & ECameraDirtyFlags::View ) )
    {
        RecalculateViewMatrix();
    }
    if ( Bool( DirtyFlags & ECameraDirtyFlags::Projection ) )
    {
        RecalculateProjectionMatrix();
    }
    if ( Bool( DirtyFlags & ECameraDirtyFlags::ViewProjection ) )
    {
        RecalculateViewProjectionMatrix();
    }

    DirtyFlags = ECameraDirtyFlags::None;
}

void LumenEngine::Maths::FCamera::RecalculateViewMatrix () noexcept
{
    ViewMatrix = FMatrix4x4f::LookAt( Position, Target, UpVector );
}

void LumenEngine::Maths::FCamera::RecalculateProjectionMatrix () noexcept
{
    if ( ProjectionMode == ECameraProjectionMode::Perspective )
    {
        const Float32 FovRadians = FieldOfView * static_cast<Float32>( DegToRad );
        ProjectionMatrix         = FMatrix4x4f::Perspective( FovRadians, AspectRatio, NearPlane, FarPlane );
    }
    else
    {
        ProjectionMatrix = FMatrix4x4f::Orthographic( FieldOfView, AspectRatio, NearPlane, FarPlane );
    }
}

void LumenEngine::Maths::FCamera::RecalculateViewProjectionMatrix () noexcept
{
    ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
}