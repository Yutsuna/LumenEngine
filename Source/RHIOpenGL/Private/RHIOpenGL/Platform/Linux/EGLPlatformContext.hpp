/**
 * @file EGLPlatformContext.hpp
 * @brief Declares the Linux EGL surfaceless OpenGL 4.6 context provider.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>

    #include <RHIOpenGL/Platform/IOpenGLPlatformContext.hpp>
    #include <RHIOpenGL/RHIOpenGLSurfaceBridge.hpp>

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
        [[nodiscard]] TExpected<void, RHI::FRHIError> Initialize ( Bool bEnableDebug,
                                                                   const FOpenGLNativeWindowDesc *InNativeWindow ) noexcept;
        [[nodiscard]] TExpected<void, RHI::FRHIError> MakeCurrent () noexcept override;
        [[nodiscard]] TOptional<UPtr> ResolveProcedure ( FStringView InName ) const noexcept override;

        /** @brief Creates a native window surface on the current EGL display and config. */
        [[nodiscard]] TExpected<UPtr, RHI::FRHIError> CreateWindowSurface () noexcept override;
        void DestroyWindowSurface ( UPtr InSurface ) noexcept override;
        [[nodiscard]] TExpected<void, RHI::FRHIError> MakeSurfaceCurrent ( UPtr InSurface ) noexcept override;
        [[nodiscard]] TExpected<void, RHI::FRHIError> SwapBuffers ( UPtr InSurface, Int32 InSwapInterval ) noexcept override;

        void Shutdown () noexcept override;

    private:

        void InitializeEglProcedure () noexcept;
        [[nodiscard]] Bool AreEglProcedureInvalid () const noexcept;

        /** @brief Releases this context's EGL objects; leaves the display ownership flags untouched. */
        void ReleaseEglObjects () noexcept;

        /** @brief Failure-path cleanup while GEglCacheMutex is already held by Initialize(). */
        void AbortInitialize () noexcept;

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
        TEGLCreateWindowSurface CreateWindow         = nullptr;
        TEGLDestroySurface DestroySurfaceProc        = nullptr;
        TEGLMakeCurrent MakeCurrentProc              = nullptr;
        TEGLSwapInterval SwapIntervalProc            = nullptr;
        TEGLSwapBuffers SwapBuffersProc              = nullptr;
        TEGLQueryString QueryStringProc              = nullptr;
        TEGLGetError GetErrorProc                    = nullptr;

        EGLDisplay Display = 0U;
        EGLConfig Config   = 0U;
        EGLContext Context = 0U;
        EGLSurface Surface = 0U;
        TOptional<FOpenGLNativeWindowDesc> NativeWindow;

        /** @brief Display was initialized here and is still owned exclusively by this context. */
        Bool bDisplayInitialized = false;

        /** @brief One reference in GCachedDisplayUsers is held by this context. */
        Bool bCachedDisplayReference = false;
    };

} // namespace RHIOpenGL

} // namespace Lumen

#endif
