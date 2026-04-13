/**
 * @file StaticMeshActor.cpp
 * @brief Implementation of the AStaticMeshActor class.
 */

#include "Actors/StaticMeshActor.hpp"
#include "Messages/EngineMessageTypes.hpp"

LumenEngine::Engine::AStaticMeshActor::AStaticMeshActor ( const ActorID InId ) noexcept : AActor( InId )
{
    /* Ctor */
}

void LumenEngine::Engine::AStaticMeshActor::Receive ( FMessage InMessage )
{
    if ( InMessage.Type == EEngineMessage::Tick )
    {
        const FTickPayload &Payload = InMessage.GetPayload<FTickPayload>();

        /** NOTE: This animation is for demonstration purposes only */
        static Float32 Angle = 0.0F;
        Angle += static_cast<Float32>( Payload.DeltaTime ) * 0.5F;
        Transform = Maths::FMatrix4x4f::RotateZ( Angle );

        if ( SceneActor.IsValid() and Mesh != nullptr and Shader != nullptr )
        {
            FDrawCommandPayload CmdPayload;
            CmdPayload.Mesh      = Mesh;
            CmdPayload.Shader    = Shader;
            CmdPayload.Transform = Transform;

            SceneActor->EnqueueMessage( FMessage::Make( EEngineMessage::TransformUpdate, GetId(), CmdPayload ) );
        }
    }
}

void LumenEngine::Engine::AStaticMeshActor::SetMeshAndShader ( Renderer::FRenderMesh *InMesh, Renderer::FRenderShader *InShader ) noexcept
{
    Mesh   = InMesh;
    Shader = InShader;
}

void LumenEngine::Engine::AStaticMeshActor::SetTransform ( const Maths::FMatrix4x4f &InTransform ) noexcept
{
    Transform = InTransform;
}

void LumenEngine::Engine::AStaticMeshActor::SetSceneActor ( const TSharedPtr<AActor> &InSceneActor ) noexcept
{
    SceneActor = InSceneActor;
}