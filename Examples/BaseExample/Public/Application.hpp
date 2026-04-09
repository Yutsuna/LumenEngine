/**
 * @file Application.hpp
 * @brief Application class declaration for the BaseExample.
 */

#pragma once

#include "Graphics/RenderResource.hpp"
#include "Graphics/Resources/RenderMesh.hpp"
#include "Graphics/Resources/RenderShader.hpp"

namespace LumenEngine
{

class FBaseApplication final
{
public:

    FBaseApplication () noexcept  = default;
    ~FBaseApplication () noexcept = default;

public:

    /** @brief Initializes the application. */
    Int32 Initialize ();

    /** @brief Ticks the application with the given delta time. */
    void Tick ( const Float64 DeltaTime );

private:

    Renderer::FRenderPacket RenderPacket;
    Renderer::FRenderMesh RenderTriangle;
    Renderer::FRenderShader RenderTriangleShader;
};

} // namespace LumenEngine
