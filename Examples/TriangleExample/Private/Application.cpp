#include "Application.hpp"

#include "Actors/SceneActor.hpp"
#include "Actors/StaticMeshActor.hpp"
#include "ErrorCodes.hpp"
#include "Generic/GenericApplication.hpp"
#include "Graphics/RenderResource.hpp"
#include "Graphics/Renderer.hpp"
#include "Logging/Logger.hpp"
#include "Maths/Vec.hpp"
#include "MessageHandler.hpp"
#include "World/World.hpp"

namespace LumenEngine
{

namespace
{

    LUMEN_LOG_DEFINE_CATEGORY( LogTriangleExample, "TriangleExample" );

    constexpr Maths::FVec3f VecV1          = { 0.0F, -0.5F, 0.0F };
    constexpr Maths::FVec3f VecV2          = { 0.5F, 0.5F, 0.0F };
    constexpr Maths::FVec3f VecV3          = { -0.5F, 0.5F, 0.0F };
    constexpr Maths::FVec3f NormalV1       = { 1.0F, 0.0F, 0.0F };
    constexpr Maths::FVec3f NormalV2       = { 0.0F, 1.0F, 0.0F };
    constexpr Maths::FVec3f NormalV3       = { 0.0F, 0.0F, 1.0F };
    constexpr Maths::FVec2f DefaultUV      = { 0.0F, 0.0F };
    constexpr Maths::FVec3f DefaultTangent = { 1.0F, 0.0F, 0.0F };

    constexpr Maths::FVertex V1 = { VecV1, NormalV1, DefaultUV, DefaultTangent };
    constexpr Maths::FVertex V2 = { VecV2, NormalV2, DefaultUV, DefaultTangent };
    constexpr Maths::FVertex V3 = { VecV3, NormalV3, DefaultUV, DefaultTangent };

    constexpr Float32 CameraFovDegrees  = 60.0F;
    constexpr Float32 CameraAspectRatio = 1280.0F / 720.0F;
    constexpr Float32 CameraNearPlane   = 0.1F;
    constexpr Float32 CameraFarPlane    = 100.0F;
    constexpr Maths::FVec3f CameraEye   = { 0.0F, 1.0F, -3.0F };

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
    CreateCamera();
    CreateActors();

    LUMEN_LOG_INFO( LogTriangleExample, "TriangleExample Initialized with MathCore abstractions." );
    return EErrorCode::Success;
}

void FTriangleExampleApplication::Tick ( const Float64 DeltaTime )
{
    World->Tick(DeltaTime);
    Camera.Tick( DeltaTime );

    const RHI::FGlobalUniformData Uniforms = { .ViewProjectionMatrix = Maths::FMatrix4x4f::Identity(),
                                               .TimeSeconds          = static_cast<Float32>( HAL::FPlatformTime::Seconds() ),
                                               .DeltaTime            = static_cast<Float32>( DeltaTime ) };

    Renderer::GRenderer->SubmitGlobalUniforms( Uniforms );
    Renderer::GRenderer->SubmitRenderPacket( RenderPacket );
}

void FTriangleExampleApplication::CreateTriangle () noexcept
{
    Triangle.Mesh.Vertices       = { V1, V2, V3 };
    Triangle.Mesh.Indices        = { 0, 1, 2 };
    Triangle.Shader.VertexPath   = "Shaders/Triangle.vert.spv";
    Triangle.Shader.FragmentPath = "Shaders/Triangle.frag.spv";
}

void FTriangleExampleApplication::CreatePacket () noexcept
{
    const Renderer::FDrawCommand DrawCommand = { .Mesh = &Triangle.Mesh, .Shader = &Triangle.Shader, .Transform = Maths::FMatrix4x4f::Identity() };

    RenderPacket.ClearColor[0] = 0.02F;
    RenderPacket.ClearColor[1] = 0.02F;
    RenderPacket.ClearColor[2] = 0.05F;
    RenderPacket.ClearColor[3] = 1.00F;
    RenderPacket.DrawCommands.emplace_back( DrawCommand );
}

void FTriangleExampleApplication::CreateCamera () noexcept
{
    Camera.SetPerspective( CameraFovDegrees, CameraAspectRatio, CameraNearPlane, CameraFarPlane );
    Camera.LookAt( CameraEye, {} );
}

void FTriangleExampleApplication::CreateActors () noexcept
{
    World = MakeUnique<Engine::FWorld>();

    TSharedRef<Engine::ASceneActor> SceneActor         = World->SpawnActor<Engine::ASceneActor>();
    TSharedRef<Engine::AStaticMeshActor> TriangleActor = World->SpawnActor<Engine::AStaticMeshActor>();

    TriangleActor->SetMeshAndShader( &Triangle.Mesh, &Triangle.Shader );
    TriangleActor->SetSceneActor( SceneActor );
}

} // namespace LumenEngine
