/**
 * @file EGLPlatformContext.hpp
 * @brief Declares the Linux EGL surfaceless OpenGL 4.6 context provider.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>

#include <RHIOpenGL/Platform/IOpenGLPlatformContext.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FEGLPlatformContext
     * @brief Headless EGL context used for capability probing and device execution.
     */
    class FEGLPlatformContext final : public IOpenGLPlatformContext, public FNonCopyable
    {
    public:

        FEGLPlatformContext () noexcept = default;
        ~FEGLPlatformContext () noexcept override;

        [[nodiscard]] TExpected<void, RHI::FRHIError> Initialize ( Bool bEnableDebug ) noexcept override;
        [[nodiscard]] TExpected<void, RHI::FRHIError> MakeCurrent () noexcept override;
        [[nodiscard]] TOptional<UPtr> ResolveProcedure ( FStringView InName ) const noexcept override;

        void Shutdown () noexcept override;

    private:

        void InitializeEglProcedure () noexcept;
        [[nodiscard]] Bool AreEglProcedureInvalid () const noexcept;

    private:

        void *Library              = nullptr;
        TEGLGetProcAddress GetProc = nullptr;

        TEGLGetDisplay GetDisplay                    = nullptr;
        TEGLGetPlatformDisplayEXT GetPlatformDisplay = nullptr;
        TEGLInitialize InitializeDisplay             = nullptr;
        TEGLTerminate TerminateDisplay               = nullptr;
        TEGLBindAPI BindAPI                          = nullptr;
        TEGLChooseConfig ChooseConfig                = nullptr;
        TEGLCreateContext CreateContextProc          = nullptr;
        TEGLDestroyContext DestroyContextProc        = nullptr;
        TEGLCreatePbufferSurface CreatePbuffer       = nullptr;
        TEGLDestroySurface DestroySurfaceProc        = nullptr;
        TEGLMakeCurrent MakeCurrentProc              = nullptr;
        TEGLQueryString QueryStringProc              = nullptr;
        TEGLGetError GetErrorProc                    = nullptr;

        EGLDisplay Display = 0U;
        EGLContext Context = 0U;
        EGLSurface Surface = 0U;
    };

} // namespace RHIOpenGL

} // namespace Lumen
