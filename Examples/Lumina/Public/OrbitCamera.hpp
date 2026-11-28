/**
 * @file OrbitCamera.hpp
 * @brief Pure orbital-camera state and math matching the WebGL reference.
 */
#pragma once

#include <Maths/Vec.hpp>

namespace Lumen
{

/** @struct FOrbitCameraState @brief Yaw, pitch, distance and fixed orbit target. */
struct FOrbitCameraState
{
    Float32 Yaw          = 0.45F;
    Float32 Pitch        = 0.22F;
    Float32 Distance     = 7.2F;
    Maths::FVec3f Target = Maths::FVec3f( 0.0F, 1.05F, 0.0F );
};

/** @struct FOrbitCameraBasis @brief Orthonormal camera basis and world position. */
struct FOrbitCameraBasis
{
    Maths::FVec3f Position;
    Maths::FVec3f Forward;
    Maths::FVec3f Right;
    Maths::FVec3f Up;
};

[[nodiscard]] FOrbitCameraState ApplyOrbitDrag ( const FOrbitCameraState &InState, Float32 InDeltaX, Float32 InDeltaY ) noexcept;
[[nodiscard]] FOrbitCameraState ApplyOrbitZoom ( const FOrbitCameraState &InState, Float32 InWheelDelta ) noexcept;
[[nodiscard]] FOrbitCameraBasis ComputeOrbitCamera ( const FOrbitCameraState &InState ) noexcept;

inline constexpr Float32 LuminaCameraFieldOfViewDegrees = 46.0F;

} // namespace Lumen
