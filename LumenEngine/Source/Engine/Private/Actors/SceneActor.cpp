/**
 * @file SceneActor.cpp
 * @brief Implementation of the ASceneActor class.
 */

#include "Actors/SceneActor.hpp"
#include "Actor/ActorTypes.hpp"

#include "Graphics/RenderResource.hpp"
#include "Graphics/Renderer.hpp"

#include "Maths/Matrix.hpp"
#include "Messages/EngineMessageTypes.hpp"
#include "RHI/RHITypes.hpp"
#include "World/SpatialRegistry.hpp"

#include <algorithm>

namespace LumenEngine
{

namespace
{

    inline const Engine::FSpatialRegistryData &SwapAndSnaphotSpatialRegistry () noexcept
    {
        Engine::FSpatialRegistry::Get().SwapReadBuffers();

        return Engine::FSpatialRegistry::Get().GetReadSnapshot();
    }

    inline void
    PushNewDrawCommand ( Renderer::FRenderPacket &Packet, const RHI::FMeshHandle &InMesh, const RHI::FPipelineHandle &InShader, const Maths::FMatrix4x4f &InTransform )
    {
        const Bool bMeshValid   = InMesh.IsValid();
        const Bool bShaderValid = InShader.IsValid();

        if ( bMeshValid and bShaderValid )
        {
            Packet.DrawCommands.emplace_back( Renderer::FDrawCommand{
                .Mesh      = InMesh,
                .Shader    = InShader,
                .Transform = InTransform,
            } );
        }
    }

    Renderer::FRenderPacket BuildRenderPacket ( const Engine::FSpatialRegistryData &InRegistryData )
    {
        Renderer::FRenderPacket Packet;

        Packet.SceneSnapshot.Transforms = InRegistryData.Transforms;
        Packet.SceneSnapshot.Meshes     = InRegistryData.Meshes;
        Packet.SceneSnapshot.Shaders    = InRegistryData.Shaders;

        Packet.DrawCommands.reserve( InRegistryData.Meshes.size() );

        const USize DrawCount = std::min( { InRegistryData.Transforms.size(), InRegistryData.Meshes.size(), InRegistryData.Shaders.size() } );

        for ( USize Index = 0U; Index < DrawCount; ++Index )
        {
            PushNewDrawCommand( Packet, InRegistryData.Meshes[Index], InRegistryData.Shaders[Index], InRegistryData.Transforms[Index] );
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

    Renderer::GRenderer->SubmitRenderPacket( BuildRenderPacket( RegistryData ) );
    PendingDraws.clear();
}
