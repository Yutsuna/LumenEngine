/**
 * @file OrbitCamera.cpp
 * @brief Implements the reference orbital-camera calculations.
 */

#include <OrbitCamera.hpp>

#include <Maths/Macros.hpp>

#include <cmath>

Lumen::FOrbitCameraState
Lumen::ApplyOrbitDrag ( const FOrbitCameraState &InState, const Float32 InDeltaX, const Float32 InDeltaY ) noexcept
{
    constexpr Float32 Sensitivity = 0.0055F;
    constexpr Float32 MinPitch    = 0.04F;
    constexpr Float32 MaxPitch    = Maths::Pi<Float32> * 0.47F;

    FOrbitCameraState Result = InState;
    Result.Yaw               = InState.Yaw - InDeltaX * Sensitivity;
    Result.Pitch             = Maths::Clamp( InState.Pitch + InDeltaY * Sensitivity, MinPitch, MaxPitch );
    return Result;
}

Lumen::FOrbitCameraState Lumen::ApplyOrbitZoom ( const FOrbitCameraState &InState, const Float32 InWheelDelta ) noexcept
{
    constexpr Float32 ZoomSpeed   = 0.0012F;
    constexpr Float32 MinDistance = 2.2F;
    constexpr Float32 MaxDistance = 18.0F;

    FOrbitCameraState Result = InState;
    Result.Distance          = Maths::Clamp( InState.Distance * std::exp( InWheelDelta * ZoomSpeed ), MinDistance, MaxDistance );
    return Result;
}

Lumen::FOrbitCameraBasis Lumen::ComputeOrbitCamera ( const FOrbitCameraState &InState ) noexcept
{
    const Float32 CosPitch = std::cos( InState.Pitch );
    const Float32 SinPitch = std::sin( InState.Pitch );
    const Float32 CosYaw   = std::cos( InState.Yaw );
    const Float32 SinYaw   = std::sin( InState.Yaw );

    FOrbitCameraBasis Basis;
    Basis.Position = Maths::FVec3f( InState.Target.X + InState.Distance * CosPitch * SinYaw,
                                    InState.Target.Y + InState.Distance * SinPitch,
                                    InState.Target.Z + InState.Distance * CosPitch * CosYaw );
    Basis.Forward  = ( InState.Target - Basis.Position ).Normalize();
    Basis.Right    = Basis.Forward.Cross( Maths::FVec3f( 0.0F, 1.0F, 0.0F ) ).Normalize();
    Basis.Up       = Basis.Right.Cross( Basis.Forward ).Normalize();
    return Basis;
}
