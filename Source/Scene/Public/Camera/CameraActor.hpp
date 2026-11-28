/**
 * @file CameraActor.hpp
 * @brief Thin message adapter that writes generic camera state into the world registries.
 */
#pragma once

#include <SceneExport.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Camera/CameraRegistry.hpp>
#include <Camera/CameraTypes.hpp>
#include <World/WorldSpatialRegistry.hpp>

namespace Lumen
{

/**
 * @class ACameraActor
 * @brief Translates standard camera messages into spatial pose and camera component updates.
 * @details Deliberately contains no input mapping, orbit policy or camera constants. Every client supplies its own
 *          view/projection through messages; the actor only converts between WorldTransform and View.
 */
class SCENE_EXPORT ACameraActor final : public AActor
{
public:

    ACameraActor ( ActorID InId, FSpatialRegistry &InRegistry, FCameraComponentRegistry &InCameras ) noexcept;

    void Receive ( const FMessage &InMessage ) noexcept override;

    LUMEN_ACTOR_BODY( ACameraActor )

private:

    /** @brief Publishes a Camera-to-World matrix and its rigidly inverted View to both registries. */
    void PublishPose ( const Maths::FMat4x4f &InWorldTransform ) noexcept;

private:

    FSpatialRegistry *Registry        = nullptr;
    FCameraComponentRegistry *Cameras = nullptr;
};

} // namespace Lumen
