/**
 * @file BasePassFeature.hpp
 * @brief Feature responsible for drawing opaque geometry.
 */

#pragma once

#include "Graphics/RenderFeature.hpp"

namespace LumenEngine
{

namespace Renderer
{

    /**
     * @class FBasePassFeature
     * @brief Standard opaque geometry pass.
     */
    class LUMEN_ENGINE_API FBasePassFeature final : public IRenderFeature
    {
    public:

        FBasePassFeature ()           = default;
        ~FBasePassFeature () override = default;

    public:

        void Initialize ( RHI::IRHI *InRHI ) override;
        void Execute ( RHI::IRHICommandList &InCmdList, const FRenderPacket &InPacket, const RHI::FGlobalUniformData &InUniforms ) override;

    private:

        void ExecuteDrawCommand ( RHI::IRHICommandList &InCmdList, const FDrawCommand &InCommand ) const;

    private:

        RHI::IRHI *RHI = nullptr;
    };

} // namespace Renderer

} // namespace LumenEngine