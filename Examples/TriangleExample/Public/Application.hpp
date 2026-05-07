/**
 * @file Application.hpp
 * @brief Declaration of the FTriangleExampleApplication class
 */

#pragma once

#include "CoreTypes.hpp"

#include "Container/SharedPtr.hpp"
#include "Container/UniquePtr.hpp"

#include "Graphics/Resources/RenderMaterial.hpp"
#include "Graphics/Resources/RenderMesh.hpp"

#include "GameApplication.hpp"

#include "Assets/AssetCompiler.hpp"
#include "World/World.hpp"

namespace LumenEngine
{

class FTriangleExampleApplication final : public IGameApplication
{
public:

    Int32 Initialize ( Int32 Argc, const AnsiChar *Argv[] ) override;
    void Tick ( const Float64 InDeltaTime ) override;
    void Shutdown () override;

private:

    void CreateResources ();
    void CreateActors ();

private:

    TUniquePtr<Engine::FWorld> World;
    TUniquePtr<Compiler::FAssetCompiler> AssetCompiler;
    TSharedPtr<Renderer::FRenderMesh> TriangleMesh;
    TSharedPtr<Renderer::FRenderMaterial> TriangleMaterial;

    FActorRef MeshActorRef;
};

} // namespace LumenEngine
