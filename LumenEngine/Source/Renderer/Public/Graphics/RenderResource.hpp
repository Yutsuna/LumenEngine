/**
 * @file RenderResource.hpp
 * @brief Base struct | class for render resources, providing an interface for initialization and cleanup.
 */

#pragma once

#include "Container/Vector.hpp"
#include "Maths/Matrix.hpp"
#include "RHI/RHITypes.hpp"

namespace LumenEngine
{

namespace Renderer
{

    struct LUMEN_ENGINE_API FDrawCommand
    {
        RHI::FMeshHandle Mesh;
        RHI::FPipelineHandle Shader;
        Maths::FMatrix4x4f Transform = Maths::FMatrix4x4f::Identity();
    };

    struct LUMEN_ENGINE_API FRenderPacket
    {
        Float32 ClearColor[4] = { 0.F, 0.F, 0.F, 1.F };
        TVector<FDrawCommand> DrawCommands;
    };

} // namespace Renderer

} // namespace LumenEngine
