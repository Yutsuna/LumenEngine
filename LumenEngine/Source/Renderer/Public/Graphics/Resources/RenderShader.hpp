/**
 * @file RenderShader.hpp
 * @brief Header for the FRenderShader class
 */

#pragma once

#include "Definitions.hpp"

#include "Container/String.hpp"
#include "RHI/RHITypes.hpp"

namespace LumenEngine
{

namespace Renderer
{

    struct LUMEN_ENGINE_API FRenderShader
    {
        FString VertexPath;
        FString FragmentPath;
        RHI::FPipelineHandle RenderHandle;
    };

} // namespace Renderer

} // namespace LumenEngine