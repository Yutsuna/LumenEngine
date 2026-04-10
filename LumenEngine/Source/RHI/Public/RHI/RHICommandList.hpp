/**
 * @file RHICommandList.hpp
 * @brief Interface for recording graphics commands agnostically.
 */

#pragma once

#include "CoreTypes.hpp"

#include "RHI/RHITypes.hpp"

namespace LumenEngine
{

namespace RHI
{

    /**
     * @class IRHICommandList
     * @brief Abstract interface representing a sequence of rendering commands.
     * @details Vulkan backend will implement this to translate agnostic calls to vkCmd*.
     */
    class LUMEN_ENGINE_API IRHICommandList
    {
    public:

        virtual ~IRHICommandList () = default;

        /** @brief Begins a dynamic rendering pass */
        virtual void BeginRendering ( const Float32 InClearColor[4] ) = 0;

        /** @brief Ends the current dynamic rendering pass */
        virtual void EndRendering () = 0;

        /** @brief Binds a graphics pipeline */
        virtual void BindPipeline ( const FPipelineHandle InPipeline ) = 0;

        /** @brief Draws an indexed mesh */
        virtual void DrawMesh ( const FMeshHandle InMesh ) = 0;

        // Future additions: BindDescriptorSet, PushConstants, SetScissor, etc.
    };

} // namespace RHI

} // namespace LumenEngine
