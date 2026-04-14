/**
 * @file Triangle.cpp
 * @brief Implementation of the ATriangle class.
 */

#include "Actors/Triangle.hpp"
#include "Messages/EngineMessageTypes.hpp"

void LumenEngine::ATriangle::Receive ( FMessage InMessage )
{

    if ( InMessage.Type == Engine::EEngineMessage::Tick )
    {
        const Engine::FTickPayload &Payload = InMessage.GetPayload<Engine::FTickPayload>();
        static Float32 Angle                = 0.0F;

        Angle += static_cast<Float32>( Payload.DeltaTime ) * 0.5F;
        Transform = Maths::FMatrix4x4f::RotateZ( Angle );

        if ( SceneActor.IsValid() and Mesh != nullptr and Shader != nullptr )
        {
            Engine::FDrawCommandPayload CmdPayload;
            CmdPayload.Mesh      = Mesh;
            CmdPayload.Shader    = Shader;
            CmdPayload.Transform = Transform;

            SceneActor->EnqueueMessage( FMessage::Make( Engine::EEngineMessage::TransformUpdate, GetId(), CmdPayload ) );
        }
    }
}
