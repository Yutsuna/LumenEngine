/**
 * @file SceneActor.cpp
 * @brief Implementation of the ASceneActor class.
 */

#include "Actors/SceneActor.hpp"
#include "Graphics/Renderer.hpp"
#include "Messages/EngineMessageTypes.hpp"
#include "World/SpatialRegistry.hpp"

LumenEngine::Engine::ASceneActor::ASceneActor ( const ActorID InId ) noexcept : AActor( InId )
{
    GetMailbox().Reserve( 512ULL );
}

void LumenEngine::Engine::ASceneActor::Receive ( const FMessage &InMessage )
{
    if ( InMessage.Type == EEngineMessage::SubmitDraw )
    {
        const FSubmitDrawPayload &Payload = InMessage.GetPayload<FSubmitDrawPayload>();
        PendingDraws.push_back( Payload.Id );
    }
    else if ( InMessage.Type == EEngineMessage::Tick )
    {
        const FTickPayload &Payload = InMessage.GetPayload<FTickPayload>();
        HandleTick( Payload.DeltaTime );
    }
}

void LumenEngine::Engine::ASceneActor::HandleTick ( const Float64 /*InDeltaTime*/ )
{
    if ( not Renderer::GRenderer.IsValid() )
    {
        return;
    }

    FSpatialRegistry::Get().SwapReadBuffers();
    const FSpatialRegistryData &Snapshot = FSpatialRegistry::Get().GetReadSnapshot();

    Renderer::FRenderPacket Packet;
    Packet.ClearColor[0] = 0.02F;
    Packet.ClearColor[1] = 0.02F;
    Packet.ClearColor[2] = 0.05F;
    Packet.ClearColor[3] = 1.00F;

    Packet.DrawCommands.reserve( PendingDraws.size() );

    for ( const ActorID PendingDrawId : PendingDraws )
    {
        const auto It = Snapshot.IDToIndex.find( PendingDrawId );

        if ( It != Snapshot.IDToIndex.end() )
        {
            const USize Index = It->second;

            if ( Snapshot.Meshes[Index].IsValid() and Snapshot.Shaders[Index].IsValid() )
            {
                Renderer::FDrawCommand Cmd;
                Cmd.Mesh      = Snapshot.Meshes[Index];
                Cmd.Shader    = Snapshot.Shaders[Index];
                Cmd.Transform = Snapshot.Transforms[Index];

                Packet.DrawCommands.push_back( Cmd );
            }
        }
    }

    Renderer::GRenderer->SubmitRenderPacket( Packet );
    PendingDraws.clear();
}
