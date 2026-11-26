/**
 * @file OpenGLFramebufferCache.hpp
 * @brief Declares the GL-thread framebuffer object cache used during replay.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Types/RHIError.hpp>

#include <Templates/Vector.hpp>

#include <RHIOpenGL/Commands/OpenGLCommandStreamOpcodes.hpp>
#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLFramebufferCache
     * @brief Owns framebuffers keyed by attachment identity, format and extent.
     * @details All native calls are performed through the device GL executor.
     */
    class FOpenGLFramebufferCache final : public FNonCopyable, public FNonMovable
    {
    public:

        explicit FOpenGLFramebufferCache ( TSharedRef<FOpenGLDeviceContext> InContext ) noexcept;

        ~FOpenGLFramebufferCache () noexcept;

    public:

        /** @return A compatible framebuffer, creating and validating it on demand. */
        [[nodiscard]] TExpected<GLuint, RHI::FRHIError> GetOrCreate ( const FOpenGLRenderingPayload &InDesc ) noexcept;

        /** @brief Unbinds the framebuffer and optionally invalidates its attachments. */
        void NotifyEndRendering ( GLuint InFramebuffer, Bool InbDiscard ) noexcept;

    private:

        struct FCacheEntry
        {
            FOpenGLRenderingPayload Desc;
            GLuint Framebuffer = 0U;
            Bool bSrgb         = false;
        };

    private:

        [[nodiscard]] static UInt32 GetColorCount ( const FOpenGLRenderingPayload &InDesc ) noexcept;
        [[nodiscard]] static GLenum SelectDepthAttachment ( GLenum InFormat ) noexcept;
        [[nodiscard]] static Bool Matches ( const FCacheEntry &InEntry, const FOpenGLRenderingPayload &InDesc ) noexcept;
        [[nodiscard]] static Bool IsSrgbFormat ( GLenum InFormat ) noexcept;

    private:

        TSharedRef<FOpenGLDeviceContext> Context;
        TVector<FCacheEntry> Entries;
    };

} // namespace RHIOpenGL

} // namespace Lumen
