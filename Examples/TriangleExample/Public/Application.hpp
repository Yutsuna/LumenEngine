/**
 * @file Application.hpp
 * @brief Application class declaration for the TriangleExample.
 */

#pragma once

#include "Graphics/RenderResource.hpp"
#include "Graphics/Resources/RenderMesh.hpp"
#include "Graphics/Resources/RenderShader.hpp"

namespace LumenEngine
{

class FTriangleExampleApplication final
{
public:

    FTriangleExampleApplication () noexcept  = default;
    ~FTriangleExampleApplication () noexcept = default;

public:

    /** @brief Initializes the application. */
    Int32 Initialize ();

    /** @brief Ticks the application with the given delta time. */
    void Tick ( const Float64 DeltaTime );

private:

    void CreateTriangle () noexcept;
    void CreatePacket () noexcept;

private:

    Renderer::FRenderMesh TriangleMesh;
    Renderer::FRenderShader TriangleShader;
    Renderer::FRenderPacket PersistentPacket;
};

} // namespace LumenEngine
