/**
 * @file RenderMaterial.hpp
 * @brief Definition of the FRenderMaterial class, representing a material resource for rendering.
 */

#pragma once

#include "Container/SharedPtr.hpp"
#include "Graphics/Resources/RenderShader.hpp"
#include "RHI/RHITypes.hpp"

namespace LumenEngine
{

namespace Renderer
{

    /**
     * @struct FRenderMaterial
     * @brief A material combines a shader with specific render state and parameters.
     */
    struct LUMEN_ENGINE_API FRenderMaterial
    {
        /** The shader used by this material. */
        TSharedPtr<FRenderShader> Shader;

        /** Optional: Specific render state overrides (Blend, Depth, etc.) could be stored here or in the Shader's Pipeline. */
        // FLumenBinaryMaterialHeader State;

        /** The handle to the underlying RHI pipeline. */
        RHI::FPipelineHandle RenderHandle;
    };

} // namespace Renderer

} // namespace LumenEngine
