/**
 * @file SceneActor.cpp
 * @brief Implementation of the ASceneActor class.
 */

#include "Actors/SceneActor.hpp"
#include "Actor/ActorTypes.hpp"

#include "Graphics/RenderResource.hpp"
#include "Graphics/Renderer.hpp"

#include "Messages/EngineMessageTypes.hpp"
#include "World/SpatialRegistry.hpp"

namespace LumenEngine
{

namespace
{

    const Engine::FSpatialRegistryData &SwapAndSnaphotSpatialRegistry () noexcept
    {
        Engine::FSpatialRegistry::Get().SwapReadBuffers();

        return Engine::FSpatialRegistry::Get().GetReadSnapshot();
    }

    TOptional<Renderer::FDrawCommand> ResolveDrawCommand ( const ActorID InId, const Engine::FSpatialRegistryData &InRegistryData )
    {
        const auto It = InRegistryData.IDToIndex.find( InId );

        if ( It == InRegistryData.IDToIndex.end() )
        {
            return {};
        }

        const USize Index       = It->second;
        const Bool bMeshValid   = InRegistryData.Meshes[Index].IsValid();
        const Bool bShaderValid = InRegistryData.Shaders[Index].IsValid();

        if ( not bMeshValid or not bShaderValid )
        {
            return {};
        }

        return Renderer::FDrawCommand{ .Mesh = InRegistryData.Meshes[Index], .Shader = InRegistryData.Shaders[Index], .Transform = InRegistryData.Transforms[Index] };
    }

    Renderer::FRenderPacket BuildRenderPacket ( const Engine::FSpatialRegistryData &InRegistryData, const TVector<ActorID> &InPendingDraws )
    {
        Renderer::FRenderPacket Packet;

        Packet.DrawCommands.reserve( InPendingDraws.size() );

        for ( const ActorID Id : InPendingDraws )
        {
            if ( const TOptional<Renderer::FDrawCommand> DrawCommand = ResolveDrawCommand( Id, InRegistryData ) )
            {
                Packet.DrawCommands.emplace_back( *DrawCommand );
            }
        }

        return Packet;
    }

} // namespace

} // namespace LumenEngine

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

    const FSpatialRegistryData &RegistryData = SwapAndSnaphotSpatialRegistry();

    Renderer::GRenderer->SubmitRenderPacket( BuildRenderPacket( RegistryData, PendingDraws ) );
    PendingDraws.clear();
}
