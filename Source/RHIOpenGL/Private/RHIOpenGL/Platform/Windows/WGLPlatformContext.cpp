/**
 * @file WGLPlatformContext.cpp
 * @brief Implements native OpenGL 4.6 context creation on Windows through WGL.
 */

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_WINDOWS )

    #include <RHIOpenGL/Platform/Windows/WGLPlatformContext.hpp>

    #include <cstring>

    #include <HAL/LockGuard.hpp>
    #include <HAL/PlatformMutex.hpp>

    #include <RHI/Types/RHIError.hpp>

    #include <Templates/Optional.hpp>

    /* Windows.h last: its macros must not leak into the engine headers above. */
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>

    #if defined( _MSC_VER )
        #pragma comment( lib, "user32.lib" )
        #pragma comment( lib, "gdi32.lib" )
    #endif

/**
 * Private
 */

namespace
{

/* WGL_ARB_create_context / WGL_ARB_create_context_profile tokens; wglext.h is deliberately not a dependency. */
constexpr Lumen::Int32 WglContextMajorVersion   = 0x2091;
constexpr Lumen::Int32 WglContextMinorVersion   = 0x2092;
constexpr Lumen::Int32 WglContextFlags          = 0x2094;
constexpr Lumen::Int32 WglContextProfileMask    = 0x9126;
constexpr Lumen::Int32 WglContextDebugBit       = 0x00000001;
constexpr Lumen::Int32 WglContextCoreProfileBit = 0x00000001;
constexpr Lumen::Int32 WglFalse                 = 0;

constexpr const wchar_t *GWglWindowClassName = L"LumenRHIOpenGLContextWindow";

/** @brief opengl32.dll and the helper window class are process-wide: loading or registering them per
 *         device is wasteful, and the class may only be unregistered once its last user is gone. */
Lumen::HAL::FPlatformMutex GWglCacheMutex;
Lumen::UPtr GCachedOpenGLLibrary = 0U;
Lumen::UInt32 GWindowClassUsers  = 0U;

LRESULT CALLBACK WglHelperWindowProc ( HWND InWindow, UINT InMessage, WPARAM InWParam, LPARAM InLParam )
{
    return ::DefWindowProcW( InWindow, InMessage, InWParam, InLParam );
}

/** @brief Module that contains this code: owns the window class, whether it is the exe or a DLL. */
inline HMODULE GetCodeModule () noexcept
{
    HMODULE Module = nullptr;
    ( void )::GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                  reinterpret_cast<LPCWSTR>( &WglHelperWindowProc ), &Module );
    return Module;
}

inline HWND ToWindow ( const Lumen::UPtr InHandle ) noexcept
{
    return reinterpret_cast<HWND>( InHandle );
}

inline HDC ToDeviceContext ( const Lumen::UPtr InHandle ) noexcept
{
    return reinterpret_cast<HDC>( InHandle );
}

template <typename THandleType> [[nodiscard]] Lumen::UPtr FromHandle ( const THandleType InHandle ) noexcept
{
    return reinterpret_cast<Lumen::UPtr>( InHandle );
}

inline Lumen::UPtr LoadWglSymbol ( const Lumen::UPtr InLibrary, const Lumen::AnsiChar *InName ) noexcept
{
    return reinterpret_cast<Lumen::UPtr>( ::GetProcAddress( reinterpret_cast<HMODULE>( InLibrary ), InName ) );
}

template <typename TProcedureType> [[nodiscard]] TProcedureType CastWglProcedure ( const Lumen::UPtr InAddress ) noexcept
{
    return reinterpret_cast<TProcedureType>( InAddress );
}

/** @brief wglGetProcAddress reports failure as 0, 1, 2, 3 or -1 depending on the driver. */
inline Lumen::UPtr NormalizeWglAddress ( const Lumen::UPtr InAddress ) noexcept
{
    return ( InAddress <= 3U or InAddress == static_cast<Lumen::UPtr>( -1 ) ) ? 0U : InAddress;
}

inline Lumen::TExpected<void, Lumen::RHI::FRHIError> WglFailure ( Lumen::FStringView InMessage ) noexcept
{
    return Lumen::TUnexpected( Lumen::RHI::MakeRHIBackendFailure( Lumen::RHI::ERHIErrorCondition::Device, InMessage ) );
}

/** @brief Opens opengl32.dll once. The default search order is kept on purpose: a software
 *         opengl32.dll (Mesa) placed next to the executable is how headless CI machines get GL 4.6. */
inline Lumen::UPtr OpenOpenGLLibrary () noexcept
{
    Lumen::HAL::TLockGuard CacheLock( GWglCacheMutex );
    if ( GCachedOpenGLLibrary == 0U )
    {
        GCachedOpenGLLibrary = FromHandle( ::LoadLibraryW( L"opengl32.dll" ) );
    }
    return GCachedOpenGLLibrary;
}

inline Lumen::Bool AcquireHelperWindowClass () noexcept
{
    Lumen::HAL::TLockGuard CacheLock( GWglCacheMutex );
    if ( GWindowClassUsers == 0U )
    {
        WNDCLASSEXW WindowClass   = {};
        WindowClass.cbSize        = sizeof( WindowClass );
        WindowClass.style         = CS_OWNDC;
        WindowClass.lpfnWndProc   = WglHelperWindowProc;
        WindowClass.hInstance     = GetCodeModule();
        WindowClass.lpszClassName = GWglWindowClassName;

        /* A previous Unregister can fail while a leaked window survives: the class is then still usable. */
        if ( ::RegisterClassExW( &WindowClass ) == 0 and ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS )
        {
            return false;
        }
    }
    ++GWindowClassUsers;
    return true;
}

inline void ReleaseHelperWindowClass () noexcept
{
    Lumen::HAL::TLockGuard CacheLock( GWglCacheMutex );
    if ( GWindowClassUsers == 0U )
    {
        return;
    }
    if ( --GWindowClassUsers == 0U )
    {
        ( void )::UnregisterClassW( GWglWindowClassName, GetCodeModule() );
    }
}

/** @brief Picks an RGBA8 / depth24 / stencil8 double-buffered format; rendering goes through FBOs, so the
 *         default framebuffer only has to be presentable. */
inline Lumen::Int32 ChooseLegacyPixelFormat ( const HDC InDeviceContext ) noexcept
{
    PIXELFORMATDESCRIPTOR Descriptor = {};
    Descriptor.nSize                 = sizeof( Descriptor );
    Descriptor.nVersion              = 1;
    Descriptor.dwFlags               = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    Descriptor.iPixelType            = PFD_TYPE_RGBA;
    Descriptor.cColorBits            = 32;
    Descriptor.cAlphaBits            = 8;
    Descriptor.cDepthBits            = 24;
    Descriptor.cStencilBits          = 8;
    Descriptor.iLayerType            = PFD_MAIN_PLANE;
    return ::ChoosePixelFormat( InDeviceContext, &Descriptor );
}

/** @brief A window accepts a pixel format exactly once. */
inline Lumen::Bool ApplyPixelFormat ( const HDC InDeviceContext, const Lumen::Int32 InPixelFormat ) noexcept
{
    PIXELFORMATDESCRIPTOR Descriptor = {};
    if ( ::DescribePixelFormat( InDeviceContext, InPixelFormat, sizeof( Descriptor ), &Descriptor ) == 0 )
    {
        return false;
    }
    return ::SetPixelFormat( InDeviceContext, InPixelFormat, &Descriptor ) != FALSE;
}

} // namespace

/**
 * Public
 */

Lumen::RHIOpenGL::FWGLPlatformContext::~FWGLPlatformContext () noexcept
{
    Shutdown();
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FWGLPlatformContext::Initialize ( const Bool bEnableDebug ) noexcept
{
    return Initialize( bEnableDebug, nullptr );
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FWGLPlatformContext::Initialize ( const Bool bEnableDebug,
                                                    const FOpenGLNativeWindowDesc *InNativeWindow ) noexcept
{
    if ( Context != 0U )
    {
        return {};
    }

    NativeWindow =
        InNativeWindow != nullptr ? TOptional<FOpenGLNativeWindowDesc>( *InNativeWindow ) : TOptional<FOpenGLNativeWindowDesc>{};

    TExpected<void, RHI::FRHIError> Result = InitializeInternal( bEnableDebug );
    if ( not Result )
    {
        /* Every step records what it owns before it can fail, so one Shutdown unwinds any partial state. */
        Shutdown();
    }
    return Result;
}

void Lumen::RHIOpenGL::FWGLPlatformContext::Shutdown () noexcept
{
    if ( Context != 0U and MakeCurrentProc != nullptr and DeleteContextProc != nullptr )
    {
        /* Only unbind when this context is the one current here; never disturb another context on this thread. */
        if ( GetCurrentContextProc != nullptr and GetCurrentContextProc() == Context )
        {
            ( void )MakeCurrentProc( 0U, 0U );
        }
        ( void )DeleteContextProc( Context );
    }
    Context = 0U;

    if ( DeviceContext != 0U and Window != 0U )
    {
        ( void )::ReleaseDC( ToWindow( Window ), ToDeviceContext( DeviceContext ) );
    }
    DeviceContext = 0U;

    if ( Window != 0U )
    {
        ( void )::DestroyWindow( ToWindow( Window ) );
    }
    Window = 0U;

    if ( bWindowClassReference )
    {
        ReleaseHelperWindowClass();
        bWindowClassReference = false;
    }

    PixelFormat              = 0;
    bSwapControlTear         = false;
    Library                  = 0U;
    GetProc                  = nullptr;
    CreateContextAttribsProc = nullptr;
    SwapIntervalProc         = nullptr;
    GetExtensionsStringProc  = nullptr;
    NativeWindow.Reset();
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHIOpenGL::FWGLPlatformContext::MakeCurrent () noexcept
{
    if ( DeviceContext == 0U or Context == 0U or MakeCurrentProc == nullptr )
    {
        return WglFailure( "WGL context is not initialized." );
    }
    if ( MakeCurrentProc( DeviceContext, Context ) == WglFalse )
    {
        return WglFailure( "Unable to make the WGL OpenGL context current." );
    }
    return {};
}

Lumen::TExpected<Lumen::RHIOpenGL::WGLSurface, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FWGLPlatformContext::CreateWindowSurface () noexcept
{
    if ( not NativeWindow or DeviceContext == 0U or PixelFormat == 0 )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Presentation,
                                                      "WGL was not initialized with a native window descriptor." ) );
    }

    const HWND WindowHandle = reinterpret_cast<HWND>( NativeWindow->NativeWindow );
    if ( WindowHandle == nullptr or ::IsWindow( WindowHandle ) == FALSE )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Presentation,
                                                      "The native OpenGL window handle is not a valid window." ) );
    }

    const HDC WindowDeviceContext = ::GetDC( WindowHandle );
    if ( WindowDeviceContext == nullptr )
    {
        return TUnexpected(
            RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Presentation, "GetDC failed for the native OpenGL window." ) );
    }

    /* A pixel format already chosen by the host is kept: it can only be set once. If the driver finds it
     * incompatible with this context, MakeSurfaceCurrent reports it on first use. */
    if ( ::GetPixelFormat( WindowDeviceContext ) == 0 and not ApplyPixelFormat( WindowDeviceContext, PixelFormat ) )
    {
        ( void )::ReleaseDC( WindowHandle, WindowDeviceContext );
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Presentation,
                                                        "SetPixelFormat failed for the native OpenGL window." ) );
    }
    return FromHandle( WindowDeviceContext );
}

void Lumen::RHIOpenGL::FWGLPlatformContext::DestroyWindowSurface ( const WGLSurface InSurface ) noexcept
{
    if ( InSurface == 0U or not NativeWindow )
    {
        return;
    }

    /* Never release a DC that is still bound on this thread: fall back to the headless DC first. */
    if ( GetCurrentDCProc != nullptr and GetCurrentDCProc() == InSurface and DeviceContext != 0U and Context != 0U and
         MakeCurrentProc != nullptr )
    {
        ( void )MakeCurrentProc( DeviceContext, Context );
    }
    ( void )::ReleaseDC( reinterpret_cast<HWND>( NativeWindow->NativeWindow ), ToDeviceContext( InSurface ) );
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FWGLPlatformContext::MakeSurfaceCurrent ( const WGLSurface InSurface ) noexcept
{
    if ( DeviceContext == 0U or Context == 0U or InSurface == 0U or MakeCurrentProc == nullptr )
    {
        return WglFailure( "WGL window surface or context is not initialized." );
    }
    if ( MakeCurrentProc( InSurface, Context ) == WglFalse )
    {
        return WglFailure( "Unable to make the native WGL window surface current." );
    }
    return {};
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FWGLPlatformContext::SwapBuffers ( const WGLSurface InSurface, const Int32 InSwapInterval ) noexcept
{
    if ( SwapIntervalProc == nullptr )
    {
        return WglFailure( "WGL native presentation entry points are unavailable." );
    }
    LUMEN_TRY_EXPECT_ERROR( MakeSurfaceCurrent( InSurface ) );

    /* Adaptive vsync (negative interval) needs WGL_EXT_swap_control_tear; otherwise fall back to plain vsync. */
    const Int32 Interval = ( InSwapInterval < 0 and not bSwapControlTear ) ? -InSwapInterval : InSwapInterval;

    const Bool bIntervalSet = SwapIntervalProc( Interval ) != WglFalse;
    const Bool bSwapped     = bIntervalSet and ::SwapBuffers( ToDeviceContext( InSurface ) ) != FALSE;

    /* The headless DC is restored on every path, including failures. */
    const TExpected<void, RHI::FRHIError> Restored = MakeCurrent();
    if ( not bIntervalSet )
    {
        return WglFailure( "wglSwapIntervalEXT rejected the requested presentation mode." );
    }
    if ( not bSwapped )
    {
        return WglFailure( "SwapBuffers failed for the native OpenGL window." );
    }
    return Restored;
}

Lumen::TOptional<Lumen::UPtr> Lumen::RHIOpenGL::FWGLPlatformContext::ResolveProcedure ( const FStringView InName ) const noexcept
{
    const FString Name( InName );

    /* wglGetProcAddress only knows extension and post-1.1 functions, and only with a context current;
     * OpenGL 1.0/1.1 functions must come straight from opengl32.dll. */
    UPtr Address = ( GetProc != nullptr ) ? NormalizeWglAddress( GetProc( Name.c_str() ) ) : 0U;

    if ( Address == 0U and Library != 0U )
    {
        Address = LoadWglSymbol( Library, Name.c_str() );
    }
    if ( Address == 0U )
    {
        return {};
    }
    return TOptional<UPtr>( Address );
}

/**
 * Private
 */

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FWGLPlatformContext::InitializeInternal ( const Bool bEnableDebug ) noexcept
{
    Library = OpenOpenGLLibrary();
    if ( Library == 0U )
    {
        return WglFailure( "Unable to open opengl32.dll." );
    }

    InitializeWglProcedure();
    if ( AreWglProcedureInvalid() )
    {
        return WglFailure( "opengl32.dll is missing a required WGL entry point." );
    }

    LUMEN_TRY_EXPECT_ERROR( CreateHelperWindow() );
    LUMEN_TRY_EXPECT_ERROR( CreateOpenGLContext( bEnableDebug ) );
    return {};
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHIOpenGL::FWGLPlatformContext::CreateHelperWindow () noexcept
{
    if ( not AcquireHelperWindowClass() )
    {
        return WglFailure( "Unable to register the OpenGL helper window class." );
    }
    bWindowClassReference = true;

    /* Never shown: the window only exists to own a device context, rendering goes through FBOs. */
    const HWND WindowHandle =
        ::CreateWindowExW( WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, GWglWindowClassName, L"Lumen OpenGL Context", WS_POPUP, 0, 0, 1,
                           1, nullptr, nullptr, GetCodeModule(), nullptr );
    if ( WindowHandle == nullptr )
    {
        return WglFailure( "Unable to create the hidden OpenGL helper window." );
    }
    Window = FromHandle( WindowHandle );

    const HDC WindowDeviceContext = ::GetDC( WindowHandle );
    if ( WindowDeviceContext == nullptr )
    {
        return WglFailure( "GetDC failed for the OpenGL helper window." );
    }
    DeviceContext = FromHandle( WindowDeviceContext );

    PixelFormat = ChooseLegacyPixelFormat( WindowDeviceContext );
    if ( PixelFormat == 0 or not ApplyPixelFormat( WindowDeviceContext, PixelFormat ) )
    {
        PixelFormat = 0;
        return WglFailure( "WGL could not provide an RGBA8/depth24 OpenGL pixel format." );
    }
    return {};
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FWGLPlatformContext::CreateOpenGLContext ( const Bool bEnableDebug ) noexcept
{
    /* wglCreateContextAttribsARB can only be resolved while some context is current, so a throwaway
     * legacy context comes first. */
    const WGLRenderContext Bootstrap = CreateContextProc( DeviceContext );
    if ( Bootstrap == 0U )
    {
        return WglFailure( "wglCreateContext failed; no OpenGL driver serves this display." );
    }
    if ( MakeCurrentProc( DeviceContext, Bootstrap ) == WglFalse )
    {
        ( void )DeleteContextProc( Bootstrap );
        return WglFailure( "Unable to make the WGL bootstrap context current." );
    }

    CreateContextAttribsProc =
        CastWglProcedure<TWGLCreateContextAttribsARB>( NormalizeWglAddress( GetProc( "wglCreateContextAttribsARB" ) ) );

    if ( CreateContextAttribsProc != nullptr )
    {
        /* Zero-terminated: index 6 ends the list unless the debug flag pair is appended. */
        Int32 ContextAttributes[9] = {
            WglContextMajorVersion, 4, WglContextMinorVersion, 6, WglContextProfileMask, WglContextCoreProfileBit, 0, 0, 0 };
        if ( bEnableDebug )
        {
            ContextAttributes[6] = WglContextFlags;
            ContextAttributes[7] = WglContextDebugBit;
        }
        Context = CreateContextAttribsProc( DeviceContext, 0U, ContextAttributes );
    }

    ( void )MakeCurrentProc( 0U, 0U );
    ( void )DeleteContextProc( Bootstrap );

    if ( CreateContextAttribsProc == nullptr )
    {
        return WglFailure( "WGL_ARB_create_context_profile is unavailable; a hardware OpenGL 4.6 driver is required." );
    }
    if ( Context == 0U )
    {
        return WglFailure( "WGL refused an OpenGL 4.6 core context; no 4.5 fallback is permitted." );
    }
    if ( MakeCurrentProc( DeviceContext, Context ) == WglFalse )
    {
        return WglFailure( "Unable to make the WGL OpenGL context current." );
    }

    /* Presentation entry points are resolved against the final context, not the bootstrap one. */
    SwapIntervalProc = CastWglProcedure<TWGLSwapIntervalEXT>( NormalizeWglAddress( GetProc( "wglSwapIntervalEXT" ) ) );
    GetExtensionsStringProc =
        CastWglProcedure<TWGLGetExtensionsStringARB>( NormalizeWglAddress( GetProc( "wglGetExtensionsStringARB" ) ) );

    if ( GetExtensionsStringProc != nullptr )
    {
        const AnsiChar *Extensions = GetExtensionsStringProc( DeviceContext );
        bSwapControlTear           = Extensions != nullptr and std::strstr( Extensions, "WGL_EXT_swap_control_tear" ) != nullptr;
    }

    if ( NativeWindow and SwapIntervalProc == nullptr )
    {
        return WglFailure( "WGL_EXT_swap_control is unavailable; native window presentation is not possible." );
    }
    return {};
}

void Lumen::RHIOpenGL::FWGLPlatformContext::InitializeWglProcedure () noexcept
{
    GetProc               = CastWglProcedure<TWGLGetProcAddress>( LoadWglSymbol( Library, "wglGetProcAddress" ) );
    CreateContextProc     = CastWglProcedure<TWGLCreateContext>( LoadWglSymbol( Library, "wglCreateContext" ) );
    DeleteContextProc     = CastWglProcedure<TWGLDeleteContext>( LoadWglSymbol( Library, "wglDeleteContext" ) );
    MakeCurrentProc       = CastWglProcedure<TWGLMakeCurrent>( LoadWglSymbol( Library, "wglMakeCurrent" ) );
    GetCurrentContextProc = CastWglProcedure<TWGLGetCurrentContext>( LoadWglSymbol( Library, "wglGetCurrentContext" ) );
    GetCurrentDCProc      = CastWglProcedure<TWGLGetCurrentDC>( LoadWglSymbol( Library, "wglGetCurrentDC" ) );
}

Lumen::Bool Lumen::RHIOpenGL::FWGLPlatformContext::AreWglProcedureInvalid () const noexcept
{
    return GetProc == nullptr or CreateContextProc == nullptr or DeleteContextProc == nullptr or MakeCurrentProc == nullptr or
           GetCurrentContextProc == nullptr or GetCurrentDCProc == nullptr;
}

#endif
