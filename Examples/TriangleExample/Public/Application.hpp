/**
 * @file Application.hpp
 * @brief Declaration of the FTriangleExampleApplication class
 */

#pragma once

#include "Container/UniquePtr.hpp"
#include "World/World.hpp"

#include "Graphics/Resources/RenderMesh.hpp"
#include "Graphics/Resources/RenderShader.hpp"

namespace LumenEngine
{

class FTriangleExampleApplication final
{
public:

    FTriangleExampleApplication () noexcept = default;

    Int32 Initialize ();
    void Tick ( const Float64 InDeltaTime );

private:

    void CreateResources ();
    void CreateActors ();

private:

    TUniquePtr<Engine::FWorld> World;
    TSharedPtr<Renderer::FRenderMesh> TriangleMesh;
    TSharedPtr<Renderer::FRenderShader> TriangleShader;
};

} // namespace LumenEngine
