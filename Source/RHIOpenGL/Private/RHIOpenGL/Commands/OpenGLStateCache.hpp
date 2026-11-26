/**
 * @file OpenGLStateCache.hpp
 * @brief Declares redundant-state elimination for replayed OpenGL state.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

#include <RHIOpenGL/Loader/GLDispatchTable.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLStateCache
     * @brief Mirrors the small set of global GL state touched by command replay.
     * @details Instances must only be used from the device GL executor thread.
     */
    class FOpenGLStateCache final : public FNonCopyable, public FNonMovable
    {
    public:

        explicit FOpenGLStateCache ( const FGLDispatchTable &InDispatch ) noexcept;

        ~FOpenGLStateCache () noexcept;

    public:

        void UseProgram ( GLuint InProgram ) noexcept;
        void BindVertexArray ( GLuint InVertexArray ) noexcept;

        void BindBufferBase ( GLenum InTarget, GLuint InIndex, GLuint InBuffer ) noexcept;
        void BindTextureUnit ( GLuint InUnit, GLuint InTexture ) noexcept;
        void BindSampler ( GLuint InUnit, GLuint InSampler ) noexcept;

        void SetViewport ( Int32 InX, Int32 InY, GLsizei InWidth, GLsizei InHeight ) noexcept;
        void SetScissor ( Bool InbEnabled, Int32 InX, Int32 InY, GLsizei InWidth, GLsizei InHeight ) noexcept;

        void Enable ( GLenum InCapability ) noexcept;
        void Disable ( GLenum InCapability ) noexcept;
        void SetEnabled ( GLenum InCapability, Bool InbEnabled ) noexcept;

        void MemoryBarrier ( GLbitfield InBarrierBits ) noexcept;

    private:

        struct FCapabilityState
        {
            GLenum Capability = GlNone;
            Bool bEnabled     = false;
        };

    private:

        [[nodiscard]] static TOptional<USize> GetTargetIndex ( GLenum InTarget ) noexcept;

    private:

        const FGLDispatchTable &Dispatch;
        GLuint CurrentProgram     = 0U;
        GLuint CurrentVertexArray = 0U;
        GLuint BoundBuffers[6][8] = {};
        GLuint BoundTextures[32]  = {};
        GLuint BoundSamplers[32]  = {};
        Int32 CurrentViewport[4]  = { 0, 0, 0, 0 };
        Int32 CurrentScissor[4]   = { 0, 0, 0, 0 };
        TVector<FCapabilityState> Capabilities;
    };

} // namespace RHIOpenGL

} // namespace Lumen
