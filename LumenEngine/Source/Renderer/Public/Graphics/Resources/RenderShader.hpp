/**
 * @file RenderShader.hpp
 * @brief Header for the FRenderShader class
 */

#pragma once

#include "Definitions.hpp"

#include "Container/String.hpp"
#include "Graphics/RenderTypes.hpp"

namespace LumenEngine
{

namespace Renderer
{

    struct LUMEN_ENGINE_API FRenderShader
    {
        FString VertexPath;
        FString FragmentPath;
        FShaderHandle RenderHandle = InvalidHandle;
    };

} // namespace Renderer

} // namespace LumenEngine
