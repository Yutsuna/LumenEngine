/**
 * @file Renderer.cpp
 * @brief Main Renderer class that manages the rendering pipeline and resources.
 */

#include "Graphics/Renderer.hpp"

#include "Container/Map.hpp"
#include "Logging/Logger.hpp"
#include "Logging/LoggingCategory.hpp"

#include <cstring>

namespace
{

const LumenEngine::FLogCategory LogRenderer( "Renderer" );

/** Opaque RHI resources stored by handle */
struct FGPU_Mesh
{
    LumenEngine::VulkanRHI::FVulkanBuffer VertexBuffer;
    LumenEngine::VulkanRHI::FVulkanBuffer IndexBuffer;
    LumenEngine::UInt32 IndexCount = 0;
};

LumenEngine::TMap<LumenEngine::Renderer::FMeshHandle, FGPU_Mesh> MeshRegistry;

} // namespace

LumenEngine::TUniquePtr<LumenEngine::Renderer::FRenderer> LumenEngine::Renderer::GRenderer = nullptr;

LumenEngine::Renderer::FRenderer::~FRenderer () noexcept
{
    Shutdown();
}

void LumenEngine::Renderer::FRenderer::Initialize ( const TSharedRef<FGenericWindow> &InWindow )
{
    RHI = MakeUnique<VulkanRHI::FVulkanRHI>();
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

    /** Clean up GPU resources through RHI abstraction */
    for ( auto &[Handle, Mesh] : MeshRegistry )
    {
        RHI->DestroyBuffer( Mesh.VertexBuffer );
        RHI->DestroyBuffer( Mesh.IndexBuffer );
    }
    MeshRegistry.clear();

    RHI->Shutdown();
    RHI.Reset();
}

void LumenEngine::Renderer::FRenderer::SubmitRenderPacket ( const FRenderPacket &InPacket )
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

    const FRenderPacket &Packet = RenderBuffer.ReadBuffer();

    if ( not RHI->BeginFrame() )
    {
        return;
    }

    RHI->BeginRendering( Packet.ClearColor );

    /** Process Draw Commands */
    for ( const FDrawCommand &Command : Packet.DrawCommands )
    {
        if ( Command.Mesh == nullptr )
        {
            continue;
        }

        /** Lazy registration of mesh resources on GPU */
        if ( Command.Mesh->RenderHandle == InvalidHandle )
        {
            static UInt32 NextHandle   = 1;
            Command.Mesh->RenderHandle = NextHandle++;

            FGPU_Mesh NewMesh;
            NewMesh.IndexCount = static_cast<UInt32>( Command.Mesh->Indices.size() );

            /** Create Vertex Buffer */
            const USize VboSize  = Command.Mesh->Vertices.size() * sizeof( Maths::FVertex );
            NewMesh.VertexBuffer = RHI->CreateBuffer( VboSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO );
            std::memcpy( NewMesh.VertexBuffer.AllocationInfo.pMappedData, Command.Mesh->Vertices.data(), VboSize );

            /** Create Index Buffer */
            const USize IboSize = Command.Mesh->Indices.size() * sizeof( UInt32 );
            NewMesh.IndexBuffer = RHI->CreateBuffer( IboSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO );
            std::memcpy( NewMesh.IndexBuffer.AllocationInfo.pMappedData, Command.Mesh->Indices.data(), IboSize );

            MeshRegistry[Command.Mesh->RenderHandle] = NewMesh;
        }

        const FGPU_Mesh &GPU_Mesh = MeshRegistry[Command.Mesh->RenderHandle];

        /** Execute Draw Call through RHI Abstraction */
        RHI->BindMeshBuffers( GPU_Mesh.VertexBuffer, GPU_Mesh.IndexBuffer );
        RHI->DrawIndexed( GPU_Mesh.IndexCount );
    }

    RHI->EndRendering();
    RHI->EndFrame();
}
