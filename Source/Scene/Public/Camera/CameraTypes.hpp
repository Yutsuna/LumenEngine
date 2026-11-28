/**
 * @file CameraTypes.hpp
 * @brief Generic camera component data, independent of any camera control policy.
 */
#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorTypes.hpp>
#include <Core/CoreTypes.hpp>
#include <Logging/LoggingCategory.hpp>
#include <Maths/Matrix.hpp>
#include <Rendering/RenderRecords.hpp>

namespace Lumen
{

/**
 * @enum ECameraComponentError
 * @brief Structured failures rejected by a camera component registry.
 */
enum class ECameraComponentError : UInt8
{
    CapacityExceeded,
    AlreadyRegistered,
    UnknownActor,
};

/**
 * @struct FCameraComponentDesc
 * @brief Optical state authored for one camera component.
 * @details View is world-to-camera, not a world transform. Projection is camera-to-clip and already
 *          encodes the aspect ratio, so no separate camera policy or orbit state lives in the component.
 */
struct SCENE_EXPORT FCameraComponentDesc final
{
    Maths::FMat4x4f View       = Maths::FMat4x4f::Identity();
    Maths::FMat4x4f Projection = Maths::FMat4x4f::Identity();
    Float32 NearPlane          = 0.1F;
    Float32 FarPlane           = 1000.0F;
    FRenderViewport Viewport{};
    ERenderCameraFlags Flags = ERenderCameraFlags::Active;
};

/**
 * @struct FCameraComponent
 * @brief One camera component owned by a weak ActorID, never by an actor object.
 */
struct SCENE_EXPORT FCameraComponent final
{
    ActorID Id = InvalidActorID;

    Maths::FMat4x4f View       = Maths::FMat4x4f::Identity();
    Maths::FMat4x4f Projection = Maths::FMat4x4f::Identity();
    Float32 NearPlane          = 0.1F;
    Float32 FarPlane           = 1000.0F;
    FRenderViewport Viewport{};
    ERenderCameraFlags Flags = ERenderCameraFlags::Active;
};

/** @brief Log category for camera component registration and capacity failures. */
extern const SCENE_EXPORT FLogCategory LogCamera;

} // namespace Lumen
