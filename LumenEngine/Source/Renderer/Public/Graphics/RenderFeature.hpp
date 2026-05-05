/**
 * @file RenderFeature.hpp
 * @brief Base interface for modular rendering passes.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Graphics/RenderResource.hpp"
#include "RHI/RHICommandList.hpp"

namespace LumenEngine
{

namespace RHI
{
    class IRHI;
}

namespace Renderer
{

    class FRenderer;
    struct FRenderPacket;

    /**
     * @class IRenderFeature
     * @brief Abstract interface representing a specific rendering pass (e.g., Opaque, PostProcess).
     */
    class LUMEN_ENGINE_API IRenderFeature
    {
    public:

        virtual ~IRenderFeature () = default;

        /**
         * @brief Called once at initialization
         * @param InRHI The RHI instance to interact with the hardware.
         */
        virtual void Initialize ( RHI::IRHI *InRHI ) = 0;

        /**
         * @brief Executes the rendering logic into the command list
         * @param InCmdList The command list to record instructions into.
         * @param InPacket The render packet containing geometry data.
         * @param InUniforms The global uniforms (Time, Matrices).
         */
        virtual void Execute ( RHI::IRHICommandList &InCmdList, const FRenderPacket &InPacket, const RHI::FGlobalUniformData &InUniforms ) = 0;
    };

} // namespace Renderer

} // namespace LumenEngine