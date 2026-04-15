/**
 * @file EngineMessageTypes.hpp
 * @brief Defines standard message types and payloads for Engine Actor communication.
 */

#pragma once

#include "CoreTypes.hpp"

#include "Maths/Matrix.hpp"
#include "Maths/Vec.hpp"

#include "Actor/ActorTypes.hpp"
#include "Graphics/RenderResource.hpp"

namespace LumenEngine
{

namespace Engine
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

        };

    } // namespace EEngineMessage

    /** @brief Payload for EEngineMessage::TransformUpdate */
    struct FTransformPayload final
    {
        Maths::FMatrix4x4f NewTransform = Maths::FMatrix4x4f::Identity();
    };

    /** @brief Payload for EEngineMessage::Tick */
    struct FTickPayload final
    {
        Float64 DeltaTime = 0.0;
    };

    /** @brief Payload for EEngineMessage::InputAxis */
    struct FInputAxisPayload final
    {
        UInt32 AxisId = 0;
        Float32 Value = 0.0F;
    };

    /** @brief Payload for the ASceneActor for rendering draws */
    struct FDrawCommandPayload final
    {
        RHI::FMeshHandle Mesh;
        RHI::FPipelineHandle Shader;
        Maths::FMatrix4x4f Transform;
    };

    /** @brief Payload for EEngineMessage::SubmitDraw */
    struct FSubmitDrawPayload final
    {
        ActorID Id = 0ULL;
    };

    /** @brief Payload for EEngineMessage::SetCameraTarget */
    struct FSetCameraTargetPayload final
    {
        ActorID TargetId           = 0;
        Maths::FVec3f TargetOffset = { 0.0F, 0.0F, 0.0F };
    };

} // namespace Engine

} // namespace LumenEngine
