/**
 * @file Camera.inl
 * @brief Inline implementations for the FCamera class.
 */

#pragma once

#include "Maths/Camera.hpp"

/**
 * Public Methods
 */

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

/**
 * Accessors
 */

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