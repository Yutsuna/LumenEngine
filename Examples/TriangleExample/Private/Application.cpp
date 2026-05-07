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
#include "Messages/EngineMessageTypes.hpp"

#ifndef LUMEN_EXAMPLE_TRIANGLE_ASSET_PATH
    #define LUMEN_EXAMPLE_TRIANGLE_ASSET_PATH ""
#endif

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogTriangleExample, "TriangleExample" );

}

/**
 * Public
 */

LumenEngine::Int32 LumenEngine::FTriangleExampleApplication::Initialize ( const Int32 LUMEN_UNUSED Argc, const AnsiChar LUMEN_UNUSED *Argv[] )
{
    if ( not GPlatformApplication.IsValid() )
    {
        return EErrorCode::Failure;
    };

    GPlatformApplication->SetMessageHandler( MakeShared<FTriangleExampleMessageHandler>() );

    World = MakeUnique<Engine::FWorld>();

    AssetCompiler = MakeUnique<Compiler::FAssetCompiler>();
    AssetCompiler->Initialize( LUMEN_EXAMPLE_TRIANGLE_ASSET_PATH );

    // TODO: make this shit automatic
    AssetCompiler->SetOnAssetReloadedCallback(
        [this] ( const FString &InPath, const Compiler::EAssetType::Type InType )
        {
            switch ( InType )
            {
            case Compiler::EAssetType::Mesh:
                if ( InPath.find( "Triangle" ) != FString::npos )
                {
                    TriangleMesh = AssetCompiler->LoadMesh( "Triangle" );
                    if ( MeshActorRef.IsValid() and TriangleMesh )
                    {
                        Engine::FMeshUpdatedPayload Payload;
                        Payload.NewMesh = TriangleMesh->RenderHandle;
                        MeshActorRef.EnqueueMessage( FMessage::Make( Engine::EEngineMessage::MeshUpdated, 0, Payload ) );
                    }
                }
                break;

            case Compiler::EAssetType::Shader:
            case Compiler::EAssetType::Material:
                TriangleMaterial = AssetCompiler->LoadMaterial( "Triangle" );
                if ( MeshActorRef.IsValid() and TriangleMaterial )
                {
                    Engine::FMaterialUpdatedPayload Payload;
                    Payload.NewMaterial = TriangleMaterial->RenderHandle;
                    MeshActorRef.EnqueueMessage( FMessage::Make( Engine::EEngineMessage::MaterialUpdated, 0, Payload ) );
                }
                break;

            default:
                break;
            }
        } );

    CreateResources();
    CreateActors();

    LUMEN_LOG_INFO( LogTriangleExample, "TriangleExample: World initialized with Camera, Scene and Mesh actors." );
    return EErrorCode::Success;
}

void LumenEngine::FTriangleExampleApplication::Shutdown ()
{
    TriangleMaterial.Reset();
    TriangleMesh.Reset();
    AssetCompiler.Reset();
    World.Reset();
}

void LumenEngine::FTriangleExampleApplication::Tick ( const Float64 InDeltaTime )
{
    AssetCompiler->Tick();
    World->Tick( InDeltaTime );
}

void LumenEngine::FTriangleExampleApplication::CreateResources ()
{
    TriangleMesh     = AssetCompiler->LoadMesh( "Triangle" );
    TriangleMaterial = AssetCompiler->LoadMaterial( "Triangle" );
}

void LumenEngine::FTriangleExampleApplication::CreateActors ()
{
    TSharedRef<Engine::ASceneActor> SceneActor  = World->SpawnActor<Engine::ASceneActor>();
    TSharedRef<AExampleCameraActor> CameraActor = World->SpawnActor<AExampleCameraActor>();
    TSharedRef<ATriangle> MeshActor             = World->SpawnActor<ATriangle>();

    MeshActorRef = MeshActor->GetRef();

    MeshActor->SetMesh( TriangleMesh );
    MeshActor->SetMaterial( TriangleMaterial );
    MeshActor->SetSceneActor( FActorRef( SceneActor.Get() ) );
}

LUMEN_REGISTER_GAME_APPLICATION( LumenEngine::FTriangleExampleApplication );
