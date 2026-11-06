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

        /** Sent by entities to update their spatial transform. Payload: FDrawCommandPayload */
        TransformUpdate,

        /** Sent by entities to request a draw call. Payload: FSubmitDrawPayload */
        SubmitDraw,

        /** Sent to cameras to bind them to a specific target actor. Payload: FSetCameraTargetPayload */
        SetCameraTarget,

        /** Sent when a mesh resource has been updated (hot-reload). Payload: FMeshUpdatedPayload */
        MeshUpdated,

        /** Sent when a material resource has been updated (hot-reload). Payload: FMaterialUpdatedPayload */
        MaterialUpdated,

    };

} // namespace EEngineMessage

/** @brief Payload for EEngineMessage::TransformUpdate */
struct SCENE_EXPORT FTransformPayload final
{
    Maths::FMatrix4x4f NewTransform = Maths::FMatrix4x4f::Identity();
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

} // namespace Lumen
