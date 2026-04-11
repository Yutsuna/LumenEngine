/**
 * @file Application.cpp
 * @brief Application class implementation for the TriangleExample.
 */

#include "Application.hpp"

#include "ErrorCodes.hpp"
#include "Generic/GenericApplication.hpp"
#include "Graphics/Renderer.hpp"
#include "Logging/Logger.hpp"
#include "MessageHandler.hpp"
#include "RHI/RHITypes.hpp"

namespace
{

constexpr LumenEngine::Maths::FVertex V1{ { 0.0F, -0.5F, 0.0F } };
constexpr LumenEngine::Maths::FVertex V2{ { 0.5F, 0.5F, 0.0F } };
constexpr LumenEngine::Maths::FVertex V3{ { -0.5F, 0.5F, 0.0F } };

} // namespace

LumenEngine::Int32 LumenEngine::FTriangleExampleApplication::Initialize ()
{

    const FLogCategory LogTriangleExample( "TriangleExample" );
    LUMEN_LOG_INFO( LogTriangleExample, "Triangle Example initializing." );

    if ( !GPlatformApplication.IsValid() )
    {
        return EErrorCode::Type::Failure;
    }

    GPlatformApplication->SetMessageHandler( MakeShared<FTriangleExampleMessageHandler>() );

    RenderTriangle.Vertices = { V1, V2, V3 };
    RenderTriangle.Indices  = { 0, 1, 2 };

    RenderTriangleShader.VertexPath   = "Shaders/Triangle.vert.spv";
    RenderTriangleShader.FragmentPath = "Shaders/Triangle.frag.spv";

    RenderPacket.ClearColor[0] = 0.05F;
    RenderPacket.ClearColor[1] = 0.05F;
    RenderPacket.ClearColor[2] = 0.15F;
    RenderPacket.ClearColor[3] = 1.00F;

    Renderer::FDrawCommand DrawTriangle;
    DrawTriangle.Mesh   = &RenderTriangle;
    DrawTriangle.Shader = &RenderTriangleShader;
    RenderPacket.DrawCommands.push_back( DrawTriangle );

    return EErrorCode::Type::Success;
}

void LumenEngine::FTriangleExampleApplication::Tick ( const Float64 DeltaTime )
{
    if ( not Renderer::GRenderer.IsValid() )
    {
        return;
    }

    Renderer::GRenderer->SubmitRenderPacket( RenderPacket );

    RHI::FGlobalUniformData Uniforms;
    Uniforms.TimeSeconds          = static_cast<Float32>( HAL::FPlatformTime::Seconds() );
    Uniforms.DeltaTime            = static_cast<Float32>( DeltaTime );
    Uniforms.ViewProjectionMatrix = Maths::FMatrix4x4f::Identity();

    Renderer::GRenderer->SubmitGlobalUniforms( Uniforms );
}
