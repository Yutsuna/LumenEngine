/**
 * @file CameraActor.hpp
 * @brief Actor-owned orbital camera and POD input messages.
 */
#pragma once

#include <Actor/Actor.hpp>
#include <OrbitCamera.hpp>
#include <World/WorldSpatialRegistry.hpp>

namespace Lumen
{

/** @enum ELuminaCameraMessage @brief Camera input messages accepted by ALuminaCameraActor. */
enum class ELuminaCameraMessage : UInt32
{
    OrbitDrag,
    OrbitZoom,
    ResetCamera,
};

/** @struct FLuminaOrbitDragMessage @brief Pixel delta of one left-button drag. */
struct FLuminaOrbitDragMessage
{
    Float32 DeltaX = 0.0F;
    Float32 DeltaY = 0.0F;
};

/** @struct FLuminaOrbitZoomMessage @brief Web-compatible wheel delta. */
struct FLuminaOrbitZoomMessage
{
    Float32 Delta = 0.0F;
};

/**
 * @class ALuminaCameraActor
 * @brief Owns orbital state and publishes only its rigid transform to the world registry.
 */
class ALuminaCameraActor final : public AActor
{
public:
    ALuminaCameraActor ( ActorID InId, FSpatialRegistry &InRegistry ) noexcept;

    void Receive ( const FMessage &InMessage ) noexcept override;
    void OnBeginPlay () noexcept override;
    void PublishTransform () noexcept;

    LUMEN_ACTOR_BODY( ALuminaCameraActor )

private:
    FSpatialRegistry *Registry = nullptr;
    FOrbitCameraState State;
};

} // namespace Lumen
