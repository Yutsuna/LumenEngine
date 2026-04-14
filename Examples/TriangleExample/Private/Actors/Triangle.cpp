/**
 * @file Triangle.cpp
 * @brief Implementation of the ATriangle class.
 */

#include "Actors/Triangle.hpp"
#include "Messages/EngineMessageTypes.hpp"

void LumenEngine::ATriangle::Receive ( const FMessage &InMessage )
{
    if ( InMessage.Type == Engine::EEngineMessage::Tick )
    {
        const Engine::FTickPayload &Payload = InMessage.GetPayload<Engine::FTickPayload>();

        RotateTriangle( Payload.DeltaTime );
        DrawTriangle();
    }
}

void LumenEngine::ATriangle::RotateTriangle ( const Float64 InDeltaTime ) noexcept
{
    static Float32 Angle = 0.0F;

    Angle += static_cast<Float32>( InDeltaTime ) * 0.5F;
    Transform = Maths::FMatrix4x4f::RotateZ( Angle );
}

void LumenEngine::ATriangle::DrawTriangle () noexcept
{
    if ( SceneActor.IsValid() and Mesh != nullptr and Shader != nullptr )
    {
        Engine::FDrawCommandPayload CmdPayload;
        CmdPayload.Mesh      = Mesh.Get();
        CmdPayload.Shader    = Shader.Get();
        CmdPayload.Transform = Transform;

        SceneActor.EnqueueMessage( FMessage::Make( Engine::EEngineMessage::TransformUpdate, GetId(), CmdPayload ) );
    }
}