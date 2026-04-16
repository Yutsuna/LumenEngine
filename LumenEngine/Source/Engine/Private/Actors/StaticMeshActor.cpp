/**
 * @file StaticMeshActor.cpp
 * @brief Implementation of the AStaticMeshActor class.
 */

#include "Actors/StaticMeshActor.hpp"
#include "Messages/EngineMessageTypes.hpp"
#include "World/SpatialRegistry.hpp"

LumenEngine::Engine::AStaticMeshActor::AStaticMeshActor ( const ActorID InId ) noexcept : ASpatialActor( InId )
{
    /* */
}

void LumenEngine::Engine::AStaticMeshActor::Receive ( const FMessage &InMessage )
{
    if ( InMessage.Type == EEngineMessage::TransformUpdate )
    {
        const FTransformPayload &Payload = InMessage.GetPayload<FTransformPayload>();
        SetTransform( Payload.NewTransform );
    }
}

void LumenEngine::Engine::AStaticMeshActor::SetMeshAndShader ( RHI::FMeshHandle InMesh, RHI::FPipelineHandle InShader ) noexcept
{
    Mesh   = InMesh;
    Shader = InShader;

    /** INFO: Registry is updated directly with the RHI handles */
    FSpatialRegistry::Get().AssignRenderData( GetId(), InMesh, InShader );
}

void LumenEngine::Engine::AStaticMeshActor::SetTransform ( const Maths::FMatrix4x4f &InTransform ) noexcept
{
    Transform = InTransform;
    FSpatialRegistry::Get().UpdateTransform( GetId(), InTransform );
}

void LumenEngine::Engine::AStaticMeshActor::SetSceneActor ( const FActorRef &InSceneActor ) noexcept
{
    SceneActor = InSceneActor;
}
