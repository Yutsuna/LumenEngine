/**
 * @file Renderer.hpp
 * @brief High-level Renderer module managing the VulkanRHI context and frame logic.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/SharedPtr.hpp"
#include "Container/UniquePtr.hpp"
#include "Thread/TripleBuffer.hpp"

#include <Vulkan/VulkanCommandBuffer.hpp>
#include <Vulkan/VulkanCommandPool.hpp>
#include <Vulkan/VulkanRHI.hpp>

#include "Graphics/RenderResource.hpp"

namespace LumenEngine
{

namespace Renderer
{

    class FGenericWindow;

    /**
     * @class FRenderer
     * @brief Renderer class responsible for managing the Vulkan context and rendering frames.
     */
    class LUMEN_ENGINE_API FRenderer
    {
    public:

        FRenderer () noexcept = default;
        ~FRenderer () noexcept;

    public:

        /**
         * @brief Initializes the renderer with the given window. This sets up the Vulkan context and prepares for rendering.
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

        /**
         * @brief Creates command buffers for each frame in flight. This is necessary for recording rendering commands.
         */
        void CreateCommandBuffers ();

    private:

        TUniquePtr<VulkanRHI::FVulkanRHI> RHI = nullptr;
        Parallel::TTripleBuffer<FRenderPacket> RenderBuffer;
    };

    extern LUMEN_ENGINE_API TUniquePtr<FRenderer> GRenderer;

} // namespace Renderer

} // namespace LumenEngine
