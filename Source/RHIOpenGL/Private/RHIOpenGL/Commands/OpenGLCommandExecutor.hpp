/**
 * @file OpenGLCommandExecutor.hpp
 * @brief Declares replay of recorded RHI command streams on the GL executor.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Commands/RHICommandStream.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Commands/OpenGLFramebufferCache.hpp>
#include <RHIOpenGL/Commands/OpenGLStateCache.hpp>
#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLCommandExecutor
     * @brief Decodes an immutable command stream into single-threaded GL calls.
     */
    class FOpenGLCommandExecutor final : public FNonCopyable, public FNonMovable
    {
    public:

        /** @brief Creates the executor and its persistent GL objects. */
        [[nodiscard]] static TExpected<TSharedRef<FOpenGLCommandExecutor>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext ) noexcept;

        explicit FOpenGLCommandExecutor ( TSharedRef<FOpenGLDeviceContext> InContext ) noexcept;

        ~FOpenGLCommandExecutor () noexcept;

    public:

        /** @brief Replays all commands in one GL-thread task. */
        [[nodiscard]] RHI::FRHIExpected Execute ( const RHI::FRHICommandStream &InStream ) noexcept;

    private:

        TSharedRef<FOpenGLDeviceContext> Context;
        TSharedRef<FOpenGLStateCache> StateCache;
        TSharedRef<FOpenGLFramebufferCache> FramebufferCache;
        GLuint VertexArray           = 0U;
        GLuint InlineConstantsBuffer = 0U;
    };

} // namespace RHIOpenGL

} // namespace Lumen
