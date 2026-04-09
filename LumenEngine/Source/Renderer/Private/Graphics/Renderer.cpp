/**
 * @file Renderer.cpp
 * @brief Main Renderer class that manages the rendering pipeline and resources.
 */

#include "Graphics/Renderer.hpp"
#include "Graphics/RenderResource.hpp"
#include "Logging/Logger.hpp"
#include "Logging/LoggingCategory.hpp"
#include "Vulkan/VulkanRHI.hpp"

namespace
{
const LumenEngine::FLogCategory LogRenderer( "Renderer" );
}

LumenEngine::TUniquePtr<LumenEngine::Renderer::FRenderer> LumenEngine::Renderer::GRenderer = nullptr;
LumenEngine::Renderer::FRenderer::FRenderer () noexcept                                    = default;

LumenEngine::Renderer::FRenderer::~FRenderer () noexcept
{
    Shutdown();
}

void LumenEngine::Renderer::FRenderer::Initialize ( const LumenEngine::TSharedRef<LumenEngine::FGenericWindow> &InWindow )
{
    RHI = LumenEngine::MakeUnique<LumenEngine::VulkanRHI::FVulkanRHI>();
    RHI->Initialize( InWindow );

    LUMEN_LOG_INFO( LogRenderer, "Renderer initialized successfully." );
}

void LumenEngine::Renderer::FRenderer::Shutdown () noexcept
{
    if ( not RHI )
    {
        return;
    }

    RHI->WaitIdle();
    RHI->Shutdown();
    RHI.Reset();
}

void LumenEngine::Renderer::FRenderer::SubmitRenderPacket ( const LumenEngine::Renderer::FRenderPacket &InPacket )
{
    RenderBuffer.WriteBuffer( InPacket );
}

void LumenEngine::Renderer::FRenderer::RenderFrame ()
{
    if ( not RHI )
    {
        return;
    }

    if ( RenderBuffer.IsDirty() )
    {
        RenderBuffer.SwapReadBuffers();
    }

    const LumenEngine::Renderer::FRenderPacket &Packet = RenderBuffer.ReadBuffer();

    if ( not RHI->BeginFrame() )
    {
        return;
    }

    for ( const LumenEngine::Renderer::FDrawCommand &Command : Packet.DrawCommands )
    {
        if ( Command.Mesh != nullptr && Command.Mesh->RenderHandle == LumenEngine::Renderer::InvalidHandle )
        {
            Command.Mesh->RenderHandle = RHI->CreateMesh( Command.Mesh->Vertices, Command.Mesh->Indices );
        }

        if ( Command.Shader != nullptr && Command.Shader->RenderHandle == LumenEngine::Renderer::InvalidHandle )
        {
            Command.Shader->RenderHandle = RHI->CreatePipeline( Command.Shader->VertexPath, Command.Shader->FragmentPath );
        }
    }

    RHI->BeginRendering( Packet.ClearColor );

    for ( const LumenEngine::Renderer::FDrawCommand &Command : Packet.DrawCommands )
    {
        if ( Command.Mesh == nullptr || Command.Shader == nullptr )
        {
            continue;
        }

        if ( Command.Shader->RenderHandle == LumenEngine::Renderer::InvalidHandle || Command.Mesh->RenderHandle == LumenEngine::Renderer::InvalidHandle )
        {
            continue;
        }

        RHI->BindPipeline( Command.Shader->RenderHandle );
        RHI->DrawMesh( Command.Mesh->RenderHandle );
    }

    RHI->EndRendering();
    RHI->EndFrame();
}
