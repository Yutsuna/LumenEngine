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

Int32 FTriangleExampleApplication::Initialize ()
{
    if ( not GPlatformApplication.IsValid() )
    {
        return EErrorCode::Failure;
    };

    GPlatformApplication->SetMessageHandler( MakeShared<FTriangleExampleMessageHandler>() );
    World = MakeUnique<Engine::FWorld>();

    CreateActors();

    LUMEN_LOG_INFO( LogTriangleExample, "TriangleExample: World initialized with Camera, Scene and Mesh actors." );
    return EErrorCode::Success;
}

void FTriangleExampleApplication::Tick ( const Float64 InDeltaTime )
{
    World->Tick( InDeltaTime );
}

void FTriangleExampleApplication::CreateActors ()
{
    TSharedRef<Engine::ASceneActor> SceneActor  = World->SpawnActor<Engine::ASceneActor>();
    TSharedRef<AExampleCameraActor> CameraActor = World->SpawnActor<AExampleCameraActor>();
    TSharedRef<ATriangleMeshActor> MeshActor    = World->SpawnActor<ATriangleMeshActor>();

    MeshActor->SetSceneActor( SceneActor );
}

} // namespace LumenEngine