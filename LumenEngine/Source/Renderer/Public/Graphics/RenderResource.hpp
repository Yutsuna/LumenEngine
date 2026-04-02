/**
 * @file RenderResource.hpp
 * @brief Base struct | class for render resources, providing an interface for initialization and cleanup.
 */

#pragma once

#include "Graphics/Resources/RenderMesh.hpp"
#include "Graphics/Resources/RenderShader.hpp"

#include "Maths/Matrix.hpp"

namespace LumenEngine
{

namespace Renderer
{

    struct LUMEN_ENGINE_API FDrawCommand
    {
        FRenderMesh *Mesh           = nullptr;
        FRenderShader *Shader       = nullptr;
        Maths::FMatrix4x4 Transform = FMatrix4x4::Identity();
    };

    struct LUMEN_ENGINE_API FRenderPacket
    {
        Float32 ClearColor[4] = { 0.F, 0.F, 0.F, 1.F };
        TVector<FDrawCommand> DrawCommands;
    };

} // namespace Renderer

} // namespace LumenEngine
