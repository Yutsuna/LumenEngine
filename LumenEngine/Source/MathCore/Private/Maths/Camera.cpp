/**
 * @file Camera.cpp
 * @brief Implementation of the FCamera class for 3D and 2D camera handling
 */

#include "Maths/Camera.hpp"
#include "Maths/Matrix.hpp"

void LumenEngine::Maths::FCamera::SetPerspective ( const Float32 InFovDegrees, const Float32 InAspectRatio, const Float32 InNear, const Float32 InFar ) noexcept
{
    FieldOfView = InFovDegrees;
    AspectRatio = InAspectRatio;
    NearPlane   = InNear;
    FarPlane    = InFar;
    DirtyFlags |= ECameraDirtyFlags::Projection | ECameraDirtyFlags::ViewProjection;
}

void LumenEngine::Maths::FCamera::LookAt ( const FVec3f &InEye, const FVec3f &InTarget, const FVec3f &InUp ) noexcept
{
    Position = InEye;
    Target   = InTarget;
    UpVector = InUp;
    DirtyFlags |= ECameraDirtyFlags::View | ECameraDirtyFlags::ViewProjection;
}

void LumenEngine::Maths::FCamera::Update () noexcept
{
    if ( DirtyFlags == ECameraDirtyFlags::None ) [[likely]]
    {
        return;
    }

    if ( Bool( DirtyFlags & ECameraDirtyFlags::View ) )
    {
        ViewMatrix = FMatrix4x4f::LookAt( Position, Target, UpVector );
    }
    if ( Bool( DirtyFlags & ECameraDirtyFlags::Projection ) )
    {
        ProjectionMatrix = FMatrix4x4f::Perspective( FieldOfView, AspectRatio, NearPlane, FarPlane );
    }
    if ( Bool( DirtyFlags & ECameraDirtyFlags::ViewProjection ) )
    {
        ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
    }

    DirtyFlags = ECameraDirtyFlags::None;
}

const LumenEngine::Maths::FMatrix4x4f &LumenEngine::Maths::FCamera::GetViewMatrix () const noexcept
{
    return ViewMatrix;
}

const LumenEngine::Maths::FMatrix4x4f &LumenEngine::Maths::FCamera::GetProjectionMatrix () const noexcept
{
    return ProjectionMatrix;
}

LumenEngine::Maths::FMatrix4x4f LumenEngine::Maths::FCamera::GetViewProjectionMatrix () const noexcept
{
    return ViewProjectionMatrix;
}
