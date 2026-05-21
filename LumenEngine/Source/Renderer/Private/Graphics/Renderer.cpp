/**
 * @file Renderer.cpp
 * @brief Implementation of the orchestrator FRenderer.
 */

#include "Graphics/Renderer.hpp"
#include "Graphics/Features/BasePassFeature.hpp"

#include "Logging/Logger.hpp"
#include "RHI/RHI.hpp"

#include "ShaderCompiler/ShaderCompiler.hpp"
#include "ShaderCompiler/ShaderCompilerRequest.hpp"

#ifndef LUMEN_GPU_CULL_SHADER_PATH
    #define LUMEN_GPU_CULL_SHADER_PATH "Assets/Shaders/GpuCulling.comp"
#endif

namespace
{

const LumenEngine::FLogCategory LogRenderer( "Renderer" );

}

LumenEngine::TUniquePtr<LumenEngine::Renderer::FRenderer> LumenEngine::Renderer::GRenderer = nullptr;

/**
 * Ctor & Dtor
 */

LumenEngine::Renderer::FRenderer::FRenderer () noexcept = default;

LumenEngine::Renderer::FRenderer::~FRenderer () noexcept
{
    Shutdown();
}

/**
 * Public
 */

void LumenEngine::Renderer::FRenderer::Initialize ( TUniquePtr<RHI::IRHI> InRHI, const TSharedRef<FGenericWindow> &InWindow )
{
    RHI = std::move( InRHI );

    if ( not RHI )
    {
        LUMEN_LOG_FATAL( LogRenderer, "Renderer initialized with a null RHI!" );
        return;
    }

    RHI->Initialize( InWindow );

    /** Initialize Shader Compiler */
    Compiler::FShaderCompilerConfig CompilerConfig;
    ShaderCompiler = MakeUnique<Compiler::FShaderCompiler>( std::move( CompilerConfig ) );

    /** Compile GPU Culling Shader */
    Compiler::FShaderCompileRequestBuilder RequestBuilder;
    RequestBuilder.Path( LUMEN_GPU_CULL_SHADER_PATH ).Compute().Macro( "MAX_INSTANCES", std::format( "{}U", RHI::MaxInstances ) );

    const Compiler::FShaderCompileResult CompileResult = ShaderCompiler->CompileShader( RequestBuilder.Build() );

    if ( CompileResult.IsSuccess() )
    {
        RHI->InitializeGpuDrivenResources( CompileResult.Shader->SpirV );
    }
    else
    {
        LUMEN_LOG_FATAL( LogRenderer, "Critical Failure: GPU Culling shader compilation failed!\nLog Output:\n{}", CompileResult.ErrorLog.c_str() );
    }

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

    if ( not RHI->BeginFrame( Uniforms ) )
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

LumenEngine::RHI::FMeshHandle LumenEngine::Renderer::FRenderer::CreateMesh ( const TVector<Maths::FVertex> &InVertices, const TVector<UInt32> &InIndices )
{
    if ( RHI )
    {
        return RHI->CreateMesh( InVertices, InIndices );
    }
    return {};
}

LumenEngine::RHI::FPipelineHandle LumenEngine::Renderer::FRenderer::CreatePipeline ( const FString &InVertexPath, const FString &InFragmentPath )
{
    if ( not RHI or not ShaderCompiler )
    {
        return {};
    }

    Compiler::FShaderCompileResult VResult = ShaderCompiler->CompileShader( Compiler::FShaderCompileRequestBuilder().Path( InVertexPath ).Vertex().Build() );
    if ( not VResult.IsSuccess() )
    {
        LUMEN_LOG_ERROR( LogRenderer, "Failed to compile vertex shader (Path: {}, Log: {}).", InVertexPath, VResult.ErrorLog.c_str() );
        return {};
    }

    Compiler::FShaderCompileResult FResult = ShaderCompiler->CompileShader( Compiler::FShaderCompileRequestBuilder().Path( InFragmentPath ).Fragment().Build() );
    if ( not FResult.IsSuccess() )
    {
        LUMEN_LOG_ERROR( LogRenderer, "Failed to compile fragment shader (Path: {}, Log: {}).", InFragmentPath, FResult.ErrorLog.c_str() );
        return {};
    }

    RHI::FGraphicsPipelineDesc Description;
    Description.VertexShader   = std::move( VResult.Shader->SpirV );
    Description.FragmentShader = std::move( FResult.Shader->SpirV );

    return RHI->CreatePipeline( Description );
}

LumenEngine::RHI::FPipelineHandle LumenEngine::Renderer::FRenderer::CreateMaterial ( const TSharedPtr<FRenderShader> &InShader )
{
    if ( RHI and InShader )
    {
        return CreatePipeline( InShader->VertexPath, InShader->FragmentPath );
    }
    return {};
}
