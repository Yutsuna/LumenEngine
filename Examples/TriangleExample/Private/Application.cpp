/**
 * @file Application.cpp
 * @brief Implementation of the FTriangleExampleApplication class
 */

#include "Application.hpp"

#include "ErrorCodes.hpp"
#include "LumenCompiler/LumenCompiler.hpp"
#include "LumenCompiler/LumenCompilerTypes.hpp"
#include "MessageHandler.hpp"

#include "Actors/Camera.hpp"
#include "Actors/SceneActor.hpp"
#include "Actors/Triangle.hpp"

#include "Generic/GenericApplication.hpp"
#include "Graphics/Renderer.hpp"

#include "Logging/Logger.hpp"

#ifndef LUMEN_EXAMPLE_TRIANGLE_SHADER_PATH
    #define LUMEN_EXAMPLE_TRIANGLE_SHADER_PATH ""
#endif
#ifndef LUMEN_EXAMPLE_TRIANGLE_ASSET_PATH
    #define LUMEN_EXAMPLE_TRIANGLE_ASSET_PATH ""
#endif

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogTriangleExample, "TriangleExample" );

}

LumenEngine::Int32 LumenEngine::FTriangleExampleApplication::Initialize ( const Int32 LUMEN_UNUSED Argc, const AnsiChar LUMEN_UNUSED *Argv[] )
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

void LumenEngine::FTriangleExampleApplication::Shutdown ()
{
    TriangleShader.Reset();
    TriangleMesh.Reset();
    World.Reset();
}

void LumenEngine::FTriangleExampleApplication::Tick ( const Float64 InDeltaTime )
{
    World->Tick( InDeltaTime );
}

namespace
{

void CompileAsset ( const LumenEngine::FString &AssetPath, const LumenEngine::FString &BlockName, const LumenEngine::FString &ExpectedBlockType ) noexcept
{
    LumenEngine::Compiler::FLumenCompiler LumenCompiler;
    LumenEngine::Compiler::FLumenCompileRequest Request;

    Request.SourcePath        = AssetPath;
    Request.TargetBlockName   = BlockName;
    Request.ExpectedBlockType = ExpectedBlockType;

    if ( const LumenEngine::Compiler::FLumenCompileResult Result = LumenCompiler.CompileAsset( Request ); Result.IsSuccess() )
    {
        LUMEN_LOG_INFO( LogTriangleExample, "Successfully compiled material asset: {}", Request.SourcePath.c_str() );
    }
    else
    {
        LUMEN_LOG_ERROR( LogTriangleExample, "Failed to compile material asset: {}. Error: {}", Request.SourcePath.c_str(), Result.ErrorLog.c_str() );
    }
}

} // namespace

void LumenEngine::FTriangleExampleApplication::CreateResources ()
{
    CompileAsset( LUMEN_EXAMPLE_TRIANGLE_ASSET_PATH "/Materials/Triangle.lumen", "TriangleMaterial", "Material" );
    CompileAsset( LUMEN_EXAMPLE_TRIANGLE_ASSET_PATH "/Meshes/Triangle.lumen", "TriangleMesh", "Mesh" );

    TriangleMesh               = MakeShared<Renderer::FRenderMesh>();
    TriangleMesh->Vertices     = { { { 0.0F, -0.5F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } },
                                   { { 0.5F, 0.5F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } },
                                   { { -0.5F, 0.5F, 0.0F }, { 0.0F, 0.0F, 1.0F }, { 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } } };
    TriangleMesh->Indices      = { 0, 1, 2 };
    TriangleMesh->RenderHandle = Renderer::GRenderer->CreateMesh( TriangleMesh->Vertices, TriangleMesh->Indices );

    TriangleShader               = MakeShared<Renderer::FRenderShader>();
    TriangleShader->VertexPath   = LUMEN_EXAMPLE_TRIANGLE_SHADER_PATH ".vert";
    TriangleShader->FragmentPath = LUMEN_EXAMPLE_TRIANGLE_SHADER_PATH ".frag";
    TriangleShader->RenderHandle = Renderer::GRenderer->CreatePipeline( TriangleShader->VertexPath, TriangleShader->FragmentPath );
}

void LumenEngine::FTriangleExampleApplication::CreateActors ()
{
    TSharedRef<Engine::ASceneActor> SceneActor  = World->SpawnActor<Engine::ASceneActor>();
    TSharedRef<AExampleCameraActor> CameraActor = World->SpawnActor<AExampleCameraActor>();
    TSharedRef<ATriangle> MeshActor             = World->SpawnActor<ATriangle>();

    MeshActor->SetMeshAndShader( TriangleMesh->RenderHandle, TriangleShader->RenderHandle );
    MeshActor->SetSceneActor( FActorRef( SceneActor.Get() ) );
}

LUMEN_REGISTER_GAME_APPLICATION( LumenEngine::FTriangleExampleApplication );
