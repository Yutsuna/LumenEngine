/**
 * @file WGLPlatformContext.hpp
 * @brief Declares the Windows WGL native OpenGL 4.6 context provider.
 */

#pragma once

#if defined( LUMEN_PLATFORM_WINDOWS )

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>

    #include <RHIOpenGL/Platform/IOpenGLPlatformContext.hpp>
    #include <RHIOpenGL/RHIOpenGLSurfaceBridge.hpp>

    #include <Templates/Optional.hpp>

    /* WGL entry points use the platform calling convention: __stdcall on x86, the single native convention elsewhere. */
    #if defined( _M_IX86 )
        #define LUMEN_WGL_CALL __stdcall
    #else
        #define LUMEN_WGL_CALL
    #endif

namespace Lumen
{

namespace RHIOpenGL
{

    /** @brief Win32 handles are stored as pointer-sized integers so <Windows.h> never leaks out of the module's .cpp files. */
    using WGLWindow        = UPtr; ///< HWND
    using WGLDeviceContext = UPtr; ///< HDC
    using WGLRenderContext = UPtr; ///< HGLRC
    using WGLSurface       = UPtr; ///< HDC of a native window; the WGL counterpart of an EGLSurface.

    using TWGLGetProcAddress          = UPtr( LUMEN_WGL_CALL * )( const AnsiChar * );
    using TWGLCreateContext           = UPtr( LUMEN_WGL_CALL * )( WGLDeviceContext );
    using TWGLDeleteContext           = Int32( LUMEN_WGL_CALL * )( WGLRenderContext );
    using TWGLMakeCurrent             = Int32( LUMEN_WGL_CALL * )( WGLDeviceContext, WGLRenderContext );
    using TWGLGetCurrentContext       = UPtr( LUMEN_WGL_CALL * )();
    using TWGLGetCurrentDC            = UPtr( LUMEN_WGL_CALL * )();
    using TWGLCreateContextAttribsARB = UPtr( LUMEN_WGL_CALL * )( WGLDeviceContext, WGLRenderContext, const Int32 * );
    using TWGLSwapIntervalEXT         = Int32( LUMEN_WGL_CALL * )( Int32 );
    using TWGLGetExtensionsStringARB  = const AnsiChar *( LUMEN_WGL_CALL * )( WGLDeviceContext );

    #undef LUMEN_WGL_CALL

    /**
     * @class FWGLPlatformContext
     * @brief Native WGL context used for capability probing and device execution.
     * @details WGL has no surfaceless mode, so the context is bound to a hidden 1x1 helper window.
     *          A native window, when requested, is exposed as a presentation "surface": its device
     *          context receives the same pixel format and the context is made current on it only
     *          for the duration of a swap.
     *
     *          Win32 ties windows and window device contexts to their creating thread:
     *          Initialize and Shutdown must run on the same thread, and so must
     *          CreateWindowSurface and DestroyWindowSurface. A context that is current on another
     *          thread cannot be deleted, so release it there before calling Shutdown.
     */
    class FWGLPlatformContext final : public IOpenGLPlatformContext, public FNonCopyable
    {
    public:

        FWGLPlatformContext () noexcept = default;
        ~FWGLPlatformContext () noexcept override;

        [[nodiscard]] TExpected<void, RHI::FRHIError> Initialize ( Bool bEnableDebug ) noexcept override;
        [[nodiscard]] TExpected<void, RHI::FRHIError> Initialize ( Bool bEnableDebug,
                                                                   const FOpenGLNativeWindowDesc *InNativeWindow ) noexcept;
        [[nodiscard]] TExpected<void, RHI::FRHIError> MakeCurrent () noexcept override;
        [[nodiscard]] TOptional<UPtr> ResolveProcedure ( FStringView InName ) const noexcept override;

        /** @brief Acquires the device context of the native window and gives it this context's pixel format. */
        [[nodiscard]] TExpected<WGLSurface, RHI::FRHIError> CreateWindowSurface () noexcept override;
        void DestroyWindowSurface ( WGLSurface InSurface ) noexcept override;
        [[nodiscard]] TExpected<void, RHI::FRHIError> MakeSurfaceCurrent ( WGLSurface InSurface ) noexcept override;
        [[nodiscard]] TExpected<void, RHI::FRHIError> SwapBuffers ( WGLSurface InSurface,
                                                                    Int32 InSwapInterval ) noexcept override;

        void Shutdown () noexcept override;

    private:

        /** @brief Body of Initialize; the caller runs Shutdown() when it fails. */
        [[nodiscard]] TExpected<void, RHI::FRHIError> InitializeInternal ( Bool bEnableDebug ) noexcept;

        /** @brief Creates the hidden helper window, its device context and its pixel format. */
        [[nodiscard]] TExpected<void, RHI::FRHIError> CreateHelperWindow () noexcept;

        /** @brief Bootstraps a legacy context, then creates the final 4.6 core context and makes it current. */
        [[nodiscard]] TExpected<void, RHI::FRHIError> CreateOpenGLContext ( Bool bEnableDebug ) noexcept;

        void InitializeWglProcedure () noexcept;
        [[nodiscard]] Bool AreWglProcedureInvalid () const noexcept;

    private:

        /** @brief opengl32.dll; process-wide and never unloaded, so it is not owned by this context. */
        UPtr Library               = 0U;
        TWGLGetProcAddress GetProc = nullptr;

        TWGLCreateContext CreateContextProc                  = nullptr;
        TWGLDeleteContext DeleteContextProc                  = nullptr;
        TWGLMakeCurrent MakeCurrentProc                      = nullptr;
        TWGLGetCurrentContext GetCurrentContextProc          = nullptr;
        TWGLGetCurrentDC GetCurrentDCProc                    = nullptr;
        TWGLCreateContextAttribsARB CreateContextAttribsProc = nullptr;
        TWGLSwapIntervalEXT SwapIntervalProc                 = nullptr;
        TWGLGetExtensionsStringARB GetExtensionsStringProc   = nullptr;

        WGLWindow Window               = 0U;
        WGLDeviceContext DeviceContext = 0U;
        WGLRenderContext Context       = 0U;
        Int32 PixelFormat              = 0;
        TOptional<FOpenGLNativeWindowDesc> NativeWindow;

        /** @brief One reference on the process-wide helper window class is held by this context. */
        Bool bWindowClassReference = false;

        /** @brief WGL_EXT_swap_control_tear is available, so negative swap intervals (adaptive vsync) are valid. */
        Bool bSwapControlTear = false;
    };

} // namespace RHIOpenGL

} // namespace Lumen

#endif
