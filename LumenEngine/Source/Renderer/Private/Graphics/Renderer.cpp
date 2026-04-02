/**
 * @file Renderer.cpp
 * @brief Main Renderer class that manages the rendering pipeline and resources.
 */

#include "Graphics/Renderer.hpp"
#include "Logging/LoggingCategory.hpp"

namespace
{

const LumenEngine::FLogCategory LogRenderer( "Renderer" );

}

LumenEngine::TUniquePtr<LumenEngine::FRenderer> LumenEngine::GRenderer = nullptr;

LumenEngine::FRenderer::~FRenderer () noexcept
{
    Shutdown();
}

void LumenEngine::FRenderer::Initialize ( const TSharedRef<FGenericWindow> &InWindow )
{
    RHI = MakeUnique<VulkanRHI::FVulkanRHI>();
    RHI->Initialize( InWindow );

    LUMEN_LOG_INFO( LogRenderer, "Renderer initialized successfully." );
}

void LumenEngine::FRenderer::Shutdown () noexcept
{
    if ( not RHI )
    {
        return;
    }

    RHI->WaitIdle();
    RHI->Shutdown();
    RHI.Reset();
}

void LumenEngine::FRenderer::SubmitRenderPacket ( const FRenderPacket &InPacket )
{
    RenderBuffer.WriteBuffer( InPacket );
}

void LumenEngine::FRenderer::RenderFrame ()
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

    RHI->ClearScreen( Packet.ClearColor );
    RHI->EndFrame();
}
