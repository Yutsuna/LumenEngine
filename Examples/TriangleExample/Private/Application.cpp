/**
 * @file Application.cpp
 * @brief Implementation of the FTriangleExampleApplication class
 */

#include "Application.hpp"

#include "ErrorCodes.hpp"
#include "MessageHandler.hpp"

#include "Actors/Camera.hpp"
#include "Actors/SceneActor.hpp"
#include "Actors/Triangle.hpp"

#include "Generic/GenericApplication.hpp"

#include "Logging/Logger.hpp"

namespace LumenEngine
{
LUMEN_LOG_DEFINE_CATEGORY( LogTriangleExample, "TriangleExample" );
}

LumenEngine::Int32 LumenEngine::FTriangleExampleApplication::Initialize ()
{
    if ( not GPlatformApplication.IsValid() )
    {
        return EErrorCode::Failure;
    };

    GPlatformApplication->SetMessageHandler( MakeShared<FTriangleExampleMessageHandler>() );
    World = MakeUnique<Engine::FWorld>();

    CreateResources();
    CreateActors();

    LUMEN_LOG_INFO( LogTriangleExample, "TriangleExample: World initialized with Camera, Scene and Mesh actors." );
    return EErrorCode::Success;
}

void LumenEngine::FTriangleExampleApplication::Tick ( const Float64 InDeltaTime )
{
    World->Tick( InDeltaTime );
}

void LumenEngine::FTriangleExampleApplication::CreateResources ()
{
    TriangleMesh           = MakeShared<Renderer::FRenderMesh>();
    TriangleMesh->Vertices = { { { 0.0F, -0.5F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } },
                               { { 0.5F, 0.5F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } },
                               { { -0.5F, 0.5F, 0.0F }, { 0.0F, 0.0F, 1.0F }, { 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } } };
    TriangleMesh->Indices  = { 0, 1, 2 };

    TriangleShader               = MakeShared<Renderer::FRenderShader>();
    TriangleShader->VertexPath   = "Shaders/Triangle.vert.spv";
    TriangleShader->FragmentPath = "Shaders/Triangle.frag.spv";
}

void LumenEngine::FTriangleExampleApplication::CreateActors ()
{
    TSharedRef<Engine::ASceneActor> SceneActor  = World->SpawnActor<Engine::ASceneActor>();
    TSharedRef<AExampleCameraActor> CameraActor = World->SpawnActor<AExampleCameraActor>();
    TSharedRef<ATriangle> MeshActor             = World->SpawnActor<ATriangle>();

    MeshActor->SetMeshAndShader( TriangleMesh, TriangleShader );
    MeshActor->SetSceneActor( FActorRef( SceneActor.Get() ) );
}
