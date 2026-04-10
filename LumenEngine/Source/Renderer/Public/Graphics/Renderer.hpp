/**
 * @file Renderer.hpp
 * @brief High-level Renderer module orchestrating the Render Graph and Features.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/SharedPtr.hpp"
#include "Container/UniquePtr.hpp"
#include "Container/Vector.hpp"

#include "Graphics/RenderFeature.hpp"
#include "Graphics/RenderResource.hpp"
#include "Thread/TripleBuffer.hpp"

namespace LumenEngine
{

class FGenericWindow;

namespace RHI
{
    class IRHI;
}

namespace Renderer
{

    class LUMEN_ENGINE_API FRenderer final
    {
    public:

        FRenderer () noexcept;
        ~FRenderer () noexcept;

    public:

        /**
         * @brief Initializes the renderer.
         * @param InRHI The injected Render Hardware Interface.
         * @param InWindow The main window.
         */
        void Initialize ( TUniquePtr<RHI::IRHI> InRHI, const TSharedRef<FGenericWindow> &InWindow );

        void Shutdown () noexcept;

        /** @brief Submits a render packet from the Game Thread. */
        void SubmitRenderPacket ( const FRenderPacket &InPacket );

        /** @brief Submits global data from the Game Thread (Camera, Time). */
        void SubmitGlobalUniforms ( const RHI::FGlobalUniformData &InUniforms );

        /** @brief Executes the render graph for the frame. */
        void RenderFrame ();

    private:

        TUniquePtr<RHI::IRHI> RHI;
        TVector<TUniquePtr<IRenderFeature>> Features;

        Parallel::TTripleBuffer<FRenderPacket> RenderBuffer;
        Parallel::TTripleBuffer<RHI::FGlobalUniformData> GlobalUniformBuffer;
    };

    extern LUMEN_ENGINE_API TUniquePtr<FRenderer> GRenderer;

} // namespace Renderer

} // namespace LumenEngine