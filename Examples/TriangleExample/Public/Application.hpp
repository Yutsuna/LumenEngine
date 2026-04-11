/**
 * @file Application.hpp
 * @brief Application class declaration for the TriangleExample.
 */

#pragma once

#include "Graphics/RenderResource.hpp"
#include "Graphics/Resources/RenderMesh.hpp"
#include "Graphics/Resources/RenderShader.hpp"

#include "Maths/Macros.hpp"
#include "Maths/Matrix.hpp"
#include "Maths/Vec.hpp"

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

    struct FTriangle final
    {
        Renderer::FRenderMesh Mesh;
        Renderer::FRenderShader Shader;
    } Triangle;
    
    struct FCamera final
    {
        const Maths::FVec3f Eye    = { 0.0F, 1.0F, -3.0F };
        const Maths::FVec3f Target = { 0.0F, 0.0F, 0.0F };
        const Maths::FVec3f Up     = { 0.0F, -1.0F, 0.0F };

        const Maths::FMatrix4x4f View       = Maths::FMatrix4x4f::LookAt( Eye, Target, Up );
        const Maths::FMatrix4x4f Projection = Maths::FMatrix4x4f::Perspective( static_cast<Float32>( Maths::HalfPi ), 1280.0F / 720.0F, 0.1F, 100.0F );
    } Camera;

    Renderer::FRenderPacket RenderPacket;
};

} // namespace LumenEngine
