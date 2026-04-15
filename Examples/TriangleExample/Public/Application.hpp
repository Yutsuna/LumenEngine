/**
 * @file Application.hpp
 * @brief Declaration of the FTriangleExampleApplication class
 */

#pragma once

#include "Container/UniquePtr.hpp"
#include "GameApplication.hpp"
#include "World/World.hpp"

#include "Graphics/Resources/RenderMesh.hpp"
#include "Graphics/Resources/RenderShader.hpp"

namespace LumenEngine
{

class FTriangleExampleApplication final : public IGameApplication
{
public:

    FTriangleExampleApplication () noexcept = default;

public:

    Int32 Initialize ( Int32 Argc, const AnsiChar *Argv[] ) override;
    void Tick ( const Float64 InDeltaTime ) override;
    void Shutdown () override;

private:

    void CreateResources ();
    void CreateActors ();

private:

    TUniquePtr<Engine::FWorld> World;
    TSharedPtr<Renderer::FRenderMesh> TriangleMesh;
    TSharedPtr<Renderer::FRenderShader> TriangleShader;
};

} // namespace LumenEngine
