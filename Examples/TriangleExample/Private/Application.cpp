#include "Application.hpp"

#include "ErrorCodes.hpp"
#include "Generic/GenericApplication.hpp"
#include "Graphics/Renderer.hpp"
#include "Logging/Logger.hpp"
#include "MessageHandler.hpp"

namespace LumenEngine
{

namespace
{

    LUMEN_LOG_DEFINE_CATEGORY( LogBaseExample, "BaseExample" );

    constexpr Maths::FVec3f VecV1 = { 0.0F, -0.5F, 0.0F };
    constexpr Maths::FVec3f VecV2 = { 0.5F, 0.5F, 0.0F };
    constexpr Maths::FVec3f VecV3 = { -0.5F, 0.5F, 0.0F };

    constexpr Maths::FVertex V1 = { VecV1 };
    constexpr Maths::FVertex V2 = { VecV2 };
    constexpr Maths::FVertex V3 = { VecV3 };

} // namespace

Int32 FTriangleExampleApplication::Initialize ()
{
    if ( not GPlatformApplication.IsValid() )
    {
        return EErrorCode::Failure;
    }

    GPlatformApplication->SetMessageHandler( MakeShared<FTriangleExampleMessageHandler>() );
    CreateTriangle();
    CreatePacket();

    LUMEN_LOG_INFO( LogBaseExample, "BaseExample Initialized with MathCore abstractions." );
    return EErrorCode::Success;
}

void FTriangleExampleApplication::Tick ( const Float64 InDeltaTime )
{
    if ( not Renderer::GRenderer.IsValid() )
    {
        return;
    }

    Renderer::GRenderer->SubmitRenderPacket( PersistentPacket );
    RHI::FGlobalUniformData GlobalData{ .ViewProjectionMatrix = Maths::FMatrix4x4f::Identity(),
                                        .TimeSeconds          = static_cast<Float32>( HAL::FPlatformTime::Seconds() ),
                                        .DeltaTime            = static_cast<Float32>( InDeltaTime ) };
    Renderer::GRenderer->SubmitGlobalUniforms( GlobalData );
}

void FTriangleExampleApplication::CreateTriangle () noexcept
{
    TriangleMesh.Vertices       = { V1, V2, V3 };
    TriangleMesh.Indices        = { 0, 1, 2 };
    TriangleShader.VertexPath   = "Shaders/Triangle.vert.spv";
    TriangleShader.FragmentPath = "Shaders/Triangle.frag.spv";
}

void FTriangleExampleApplication::CreatePacket () noexcept
{
    PersistentPacket.ClearColor[0] = 0.02F;
    PersistentPacket.ClearColor[1] = 0.02F;
    PersistentPacket.ClearColor[2] = 0.05F;
    PersistentPacket.ClearColor[3] = 1.00F;
    PersistentPacket.DrawCommands.emplace_back( &TriangleMesh, &TriangleShader, Maths::FMatrix4x4f::Identity() );
}

} // namespace LumenEngine
