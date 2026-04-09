/**
 * @file Renderer.hpp
 * @brief High-level Renderer module managing the RHI context and frame logic.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/SharedPtr.hpp"
#include "Container/UniquePtr.hpp"
#include "Graphics/RenderResource.hpp"
#include "Thread/TripleBuffer.hpp"

namespace LumenEngine
{

class FGenericWindow;

namespace VulkanRHI
{
    class FVulkanRHI;
}

namespace Renderer
{

    /**
     * @class FRenderer
     * @brief Renderer class responsible for managing the RHI context and rendering frames.
     */
    class LUMEN_ENGINE_API FRenderer
    {
    public:

        /** Constructor declared here, defined in .cpp to avoid incomplete type errors with TUniquePtr */
        FRenderer () noexcept;
        ~FRenderer () noexcept;

    public:

        /**
         * @brief Initializes the renderer with the given window.
         * @param InWindow The window to render to.
         */
        void Initialize ( const TSharedRef<FGenericWindow> &InWindow );

        /** @brief Shuts down the renderer and releases all resources. */
        void Shutdown () noexcept;

        /** @brief Submits a render packet from the Game Thread to the Render Thread. */
        void SubmitRenderPacket ( const FRenderPacket &InPacket );

        /** @brief Renders a single frame. */
        void RenderFrame ();

    private:

        TUniquePtr<VulkanRHI::FVulkanRHI> RHI;
        Parallel::TTripleBuffer<FRenderPacket> RenderBuffer;
    };

    extern LUMEN_ENGINE_API TUniquePtr<FRenderer> GRenderer;

} // namespace Renderer

} // namespace LumenEngine
