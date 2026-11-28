/**
 * @file EngineMessageTypes.hpp
 * @brief Defines standard message types and payloads for Engine Actor communication.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <SceneExport.hpp>

#include <Maths/Matrix.hpp>
#include <Maths/Vec.hpp>

#include <Actor/ActorTypes.hpp>

namespace Lumen
{

/**
 * @enum EEngineMessage
 * @brief Standardized message types for engine-level actors.
 */
namespace EEngineMessage
{

    enum Type : UInt8
    {
        /** Sent every frame to tick the actor. Payload: FTickPayload */
        Tick = 100,

        /** Sent by the input system when an axis changes. Payload: FInputAxisPayload */
        InputAxis,

        /** Sent by the input system when an action is triggered. Payload: FInputActionPayload */
        InputAction,

        /** Sent by entities to update their spatial transform. Payload: FTransformPayload */
        TransformUpdate,

        /** Sent by entities to request a draw call. Payload: FSubmitDrawPayload */
        SubmitDraw,

        /** Sent to cameras to bind them to a specific target actor. Payload: FSetCameraTargetPayload */
        SetCameraTarget,

        /** Sent when a mesh resource has been updated (hot-reload). Payload: FMeshUpdatedPayload */
        MeshUpdated,

        /** Sent when a material resource has been updated (hot-reload). Payload: FMaterialUpdatedPayload */
        MaterialUpdated,

        /** Replaces the full camera world transform. Payload: FCameraPosePayload */
        CameraSetPose,

        /** Aims a camera from an eye point. Payload: FCameraLookAtPayload */
        CameraSetLookAt,

        /** Rebuilds the projection from field of view, aspect and clipping planes. Payload: FCameraParamsPayload */
        CameraSetParams,

        /** Enables or disables camera selection. Payload: FCameraActivePayload */
        CameraSetActive,
    };

} // namespace EEngineMessage

/** @brief Payload for EEngineMessage::TransformUpdate */
struct SCENE_EXPORT FTransformPayload final
{
    Maths::FMat4x4f NewTransform = Maths::FMat4x4f::Identity();
};

/** @brief Payload for EEngineMessage::Tick */
struct SCENE_EXPORT FTickPayload final
{
    Float64 DeltaTime = 0.0;
};

/** @brief Payload for EEngineMessage::InputAxis */
struct SCENE_EXPORT FInputAxisPayload final
{
    UInt32 AxisId = 0;
    Float32 Value = 0.0F;
};

/** @brief Payload for EEngineMessage::InputAction */
struct SCENE_EXPORT FInputActionPayload final
{
    UInt32 ActionId = 0;
};

/** @brief Payload for EEngineMessage::MeshUpdated */
struct SCENE_EXPORT FMeshUpdatedPayload final
{
    UInt64 ResourceId = 0ULL;
};

/** @brief Payload for EEngineMessage::MaterialUpdated */
struct SCENE_EXPORT FMaterialUpdatedPayload final
{
    UInt64 ResourceId = 0ULL;
};

/** @brief Payload for EEngineMessage::SubmitDraw */
struct SCENE_EXPORT FSubmitDrawPayload final
{
    ActorID Id = 0ULL;
};

/** @brief Payload for EEngineMessage::SetCameraTarget */
struct SCENE_EXPORT FSetCameraTargetPayload final
{
    ActorID TargetId           = 0;
    Maths::FVec3f TargetOffset = { 0.0F, 0.0F, 0.0F };
};

/** @brief Payload for EEngineMessage::CameraSetPose; Camera-to-World, never View. */
struct SCENE_EXPORT FCameraPosePayload final
{
    Maths::FMat4x4f WorldTransform = Maths::FMat4x4f::Identity();
};

/** @brief Payload for EEngineMessage::CameraSetLookAt. */
struct SCENE_EXPORT FCameraLookAtPayload final
{
    Maths::FVec3f Eye    = { 0.0F, 0.0F, 0.0F };
    Maths::FVec3f Target = { 0.0F, 0.0F, 0.0F };
    Maths::FVec3f Up     = { 0.0F, 1.0F, 0.0F };
};

/** @brief Payload for EEngineMessage::CameraSetParams. */
struct SCENE_EXPORT FCameraParamsPayload final
{
    Float32 FieldOfViewDegrees = 60.0F;
    Float32 AspectRatio        = 1.0F;
    Float32 NearPlane          = 0.1F;
    Float32 FarPlane           = 1000.0F;
};

/** @brief Payload for EEngineMessage::CameraSetActive. */
struct SCENE_EXPORT FCameraActivePayload final
{
    Bool bActive = true;
};

} // namespace Lumen
