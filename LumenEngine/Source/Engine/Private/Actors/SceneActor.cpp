/**
 * @file SceneActor.cpp
 * @brief Implementation of the ASceneActor class.
 */

#include "Actors/SceneActor.hpp"
#include "Graphics/Renderer.hpp"
#include "Messages/EngineMessageTypes.hpp"

LumenEngine::Engine::ASceneActor::ASceneActor ( const ActorID InId ) noexcept : AActor( InId )
{
    /* */
}

void LumenEngine::Engine::ASceneActor::Receive ( const FMessage &InMessage )
{
    if ( InMessage.Type == EEngineMessage::TransformUpdate )
    {
        const FDrawCommandPayload &Payload = InMessage.GetPayload<FDrawCommandPayload>();

        Renderer::FDrawCommand &Cmd = PendingDraws[InMessage.Sender];
        Cmd.Mesh                    = Payload.Mesh;
        Cmd.Shader                  = Payload.Shader;
        Cmd.Transform               = Payload.Transform;
    }
    else if ( InMessage.Type == EEngineMessage::Tick )
    {
        const FTickPayload &Payload = InMessage.GetPayload<FTickPayload>();
        HandleTick( Payload.DeltaTime );
    }
}

void LumenEngine::Engine::ASceneActor::HandleTick ( const Float64 /*InDeltaTime*/ ) const
{
    if ( not Renderer::GRenderer.IsValid() )
    {
        return;
    }

    Renderer::FRenderPacket Packet;
    Packet.ClearColor[0] = 0.02F;
    Packet.ClearColor[1] = 0.02F;
    Packet.ClearColor[2] = 0.05F;
    Packet.ClearColor[3] = 1.00F;

    Packet.DrawCommands.reserve( PendingDraws.size() );

    for ( const auto &[_, DrawCmd] : PendingDraws )
    {
        Packet.DrawCommands.push_back( DrawCmd );
    }

    Renderer::GRenderer->SubmitRenderPacket( Packet );
}