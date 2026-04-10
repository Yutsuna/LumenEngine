/**
 * @file Renderer.cpp
 * @brief Implementation of the orchestrator FRenderer.
 */

#include "Graphics/Renderer.hpp"
#include "Graphics/Features/BasePassFeature.hpp"

#include "Logging/Logger.hpp"
#include "RHI/RHI.hpp"

namespace
{
const LumenEngine::FLogCategory LogRenderer( "Renderer" );
}

LumenEngine::TUniquePtr<LumenEngine::Renderer::FRenderer> LumenEngine::Renderer::GRenderer = nullptr;

LumenEngine::Renderer::FRenderer::FRenderer () noexcept = default;

LumenEngine::Renderer::FRenderer::~FRenderer () noexcept
{
    Shutdown();
}

void LumenEngine::Renderer::FRenderer::Initialize ( TUniquePtr<RHI::IRHI> InRHI, const TSharedRef<FGenericWindow> &InWindow )
{
    RHI = std::move( InRHI );

    if ( not RHI )
    {
        LUMEN_LOG_FATAL( LogRenderer, "Renderer initialized with a null RHI!" );
        return;
    }

    RHI->Initialize( InWindow );

    TUniquePtr<FBasePassFeature> BasePass = MakeUnique<FBasePassFeature>();
    BasePass->Initialize( RHI.Get() );
    Features.emplace_back( std::move( BasePass ) );

    LUMEN_LOG_INFO( LogRenderer, "Renderer initialized successfully with Modular Features." );
}

void LumenEngine::Renderer::FRenderer::Shutdown () noexcept
{
    if ( not RHI )
    {
        return;
    }

    RHI->WaitIdle();
    Features.clear();
    RHI->Shutdown();
    RHI.Reset();
}

void LumenEngine::Renderer::FRenderer::SubmitRenderPacket ( const FRenderPacket &InPacket )
{
    RenderBuffer.WriteBuffer( InPacket );
}

void LumenEngine::Renderer::FRenderer::SubmitGlobalUniforms ( const RHI::FGlobalUniformData &InUniforms )
{
    GlobalUniformBuffer.WriteBuffer( InUniforms );
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

    if ( GlobalUniformBuffer.IsDirty() )
    {
        GlobalUniformBuffer.SwapReadBuffers();
    }

    const FRenderPacket &Packet             = RenderBuffer.ReadBuffer();
    const RHI::FGlobalUniformData &Uniforms = GlobalUniformBuffer.ReadBuffer();

    if ( not RHI->BeginFrame() )
    {
        return;
    }

    RHI::IRHICommandList &CmdList = RHI->GetCommandList();

    for ( const TUniquePtr<IRenderFeature> &Feature : Features )
    {
        Feature->Execute( CmdList, Packet, Uniforms );
    }

    RHI->EndFrame();
}