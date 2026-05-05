/**
 * @file StaticMeshActor.cpp
 * @brief Implementation of the AStaticMeshActor class.
 */

#include "Actors/StaticMeshActor.hpp"
#include "Messages/EngineMessageTypes.hpp"
#include "World/SpatialRegistry.hpp"

/**
 * Ctor
 */

LumenEngine::Engine::AStaticMeshActor::AStaticMeshActor ( const ActorID InId ) noexcept : ASpatialActor( InId )
{
    /* */
}

/**
 * Public
 */

void LumenEngine::Engine::AStaticMeshActor::Receive ( const FMessage &InMessage )
{
    switch ( InMessage.Type )
    {
    case EEngineMessage::TransformUpdate:
    {
        const FTransformPayload &Payload = InMessage.GetPayload<FTransformPayload>();
        SetTransform( Payload.NewTransform );
        break;
    }
    case EEngineMessage::MeshUpdated:
    {
        const FMeshUpdatedPayload &Payload = InMessage.GetPayload<FMeshUpdatedPayload>();
        SetMeshAndShader( Payload.NewMesh, Shader );
        break;
    }
    case EEngineMessage::MaterialUpdated:
    {
        const FMaterialUpdatedPayload &Payload = InMessage.GetPayload<FMaterialUpdatedPayload>();
        SetMeshAndShader( Mesh, Payload.NewMaterial );
        break;
    }
    default:
        break;
    }
}

void LumenEngine::Engine::AStaticMeshActor::SetMeshAndShader ( RHI::FMeshHandle InMesh, RHI::FPipelineHandle InShader ) noexcept
{
    Mesh   = InMesh;
    Shader = InShader;

    /** INFO: Registry is updated directly with the RHI handles */
    FSpatialRegistry::Get().AssignRenderData( GetId(), InMesh, InShader );
}

void LumenEngine::Engine::AStaticMeshActor::SetMaterial ( const TSharedPtr<Renderer::FRenderMaterial> &InMaterial ) noexcept
{
    if ( InMaterial )
    {
        Shader = InMaterial->RenderHandle;
        FSpatialRegistry::Get().AssignRenderData( GetId(), Mesh, Shader );
    }
}

void LumenEngine::Engine::AStaticMeshActor::SetMesh ( const TSharedPtr<Renderer::FRenderMesh> &InMesh ) noexcept
{
    if ( InMesh )
    {
        Mesh = InMesh->RenderHandle;
        FSpatialRegistry::Get().AssignRenderData( GetId(), Mesh, Shader );
    }
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
