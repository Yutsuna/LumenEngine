/**
 * @file StaticMeshActor.cpp
 * @brief Implementation of the AStaticMeshActor class.
 */

#include "Actors/StaticMeshActor.hpp"

LumenEngine::Engine::AStaticMeshActor::AStaticMeshActor ( const ActorID InId ) noexcept : AActor( InId )
{
    /* Ctor */
}

void LumenEngine::Engine::AStaticMeshActor::Receive ( const FMessage & /* InMessage */ )
{
}

void LumenEngine::Engine::AStaticMeshActor::SetMeshAndShader ( TSharedPtr<Renderer::FRenderMesh> InMesh, TSharedPtr<Renderer::FRenderShader> InShader ) noexcept
{
    Mesh   = InMesh;
    Shader = InShader;
}

void LumenEngine::Engine::AStaticMeshActor::SetTransform ( const Maths::FMatrix4x4f &InTransform ) noexcept
{
    Transform = InTransform;
}

void LumenEngine::Engine::AStaticMeshActor::SetSceneActor ( const FActorRef &InSceneActor ) noexcept
{
    SceneActor = InSceneActor;
}