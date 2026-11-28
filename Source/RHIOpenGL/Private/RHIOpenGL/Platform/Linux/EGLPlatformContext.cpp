/**
 * @file EGLPlatformContext.cpp
 * @brief Implements headless EGL context creation on Linux.
 */

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )

#include <RHIOpenGL/Platform/Linux/EGLPlatformContext.hpp>

#include <dlfcn.h>

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformMutex.hpp>

#include <RHI/Types/RHIError.hpp>

#include <String/String.hpp>
#include <Templates/Map.hpp>
#include <Templates/Optional.hpp>

/**
 * Private
 */

namespace
{

inline Lumen::UPtr LoadEglSymbol ( void *InLibrary, const Lumen::AnsiChar *InName ) noexcept
{
    return reinterpret_cast<Lumen::UPtr>( ::dlsym( InLibrary, InName ) );
}

template <typename TProcedureType> [[nodiscard]] TProcedureType CastEglProcedure ( const Lumen::UPtr InAddress ) noexcept
{
    return reinterpret_cast<TProcedureType>( InAddress );
}

inline Lumen::TExpected<void, Lumen::RHI::FRHIError> EglFailure ( Lumen::FStringView InMessage ) noexcept
{
    return Lumen::TUnexpected( Lumen::RHI::MakeRHIBackendFailure( Lumen::RHI::ERHIErrorCondition::Device, InMessage ) );
}

/** @brief EGL's library and default display are process-wide; re-creating them per
 *         device exhausts Mesa's display slots after a few dozen devices. */
Lumen::HAL::FPlatformMutex GEglCacheMutex;
void *GCachedEglLibrary                        = nullptr;
Lumen::RHIOpenGL::EGLDisplay GCachedEglDisplay = 0U;
Lumen::TMap<Lumen::UPtr, Lumen::RHIOpenGL::EGLDisplay> GCachedNativeEglDisplays;

/** @brief Live FEGLPlatformContext users per cached display; the last one must eglTerminate it. */
Lumen::TMap<Lumen::RHIOpenGL::EGLDisplay, Lumen::UInt32> GCachedDisplayUsers;

/** @brief Opens libEGL once; the caller must hold GEglCacheMutex. */
inline void *OpenEglLocked () noexcept
{
    if ( GCachedEglLibrary != nullptr )
    {
        return GCachedEglLibrary;
    }
    void *Library = ::dlopen( "libEGL.so.1", RTLD_NOW | RTLD_LOCAL );

    if ( Library == nullptr )
    {
        Library = ::dlopen( "libEGL.so", RTLD_NOW | RTLD_LOCAL );
    }
    GCachedEglLibrary = Library;
    return Library;
}

} // namespace

/**
 * Public
 */

Lumen::RHIOpenGL::FEGLPlatformContext::~FEGLPlatformContext () noexcept
{
    Shutdown();
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FEGLPlatformContext::Initialize ( const Bool bEnableDebug ) noexcept
{
    return Initialize( bEnableDebug, nullptr );
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FEGLPlatformContext::Initialize ( const Bool bEnableDebug,
                                                    const FOpenGLNativeWindowDesc *InNativeWindow ) noexcept
{
    if ( Library != nullptr )
    {
        return {};
    }

    NativeWindow =
        InNativeWindow != nullptr ? TOptional<FOpenGLNativeWindowDesc>( *InNativeWindow ) : TOptional<FOpenGLNativeWindowDesc>{};

    /* One process-wide lock covers cached handles: a single thread performs
     * dlopen/eglInitialize and publishes the result to every other device. */
    Lumen::HAL::TLockGuard CacheLock( GEglCacheMutex );
    Library = OpenEglLocked();
    if ( Library == nullptr )
    {
        return EglFailure( "Unable to open libEGL (libEGL.so.1 / libEGL.so)." );
    }

    InitializeEglProcedure();

    if ( AreEglProcedureInvalid() )
    {
        AbortInitialize();
        return EglFailure( "libEGL is missing a required core entry point." );
    }

    if ( NativeWindow and ( CreateWindow == nullptr or SwapIntervalProc == nullptr or SwapBuffersProc == nullptr ) )
    {
        AbortInitialize();
        return EglFailure( "libEGL is missing native window presentation entry points." );
    }

    Bool bInitializeDisplay = false;
    if ( NativeWindow )
    {
        const auto Cached  = GCachedNativeEglDisplays.find( NativeWindow->NativeDisplay );
        bInitializeDisplay = Cached == GCachedNativeEglDisplays.end();
        Display            = bInitializeDisplay ? GetDisplay( NativeWindow->NativeDisplay ) : Cached->second;
    }
    else
    {
        bInitializeDisplay = GCachedEglDisplay == 0U;
        GetPlatformDisplay =
            CastEglProcedure<TEGLGetPlatformDisplayEXT>( reinterpret_cast<UPtr>( GetProc( "eglGetPlatformDisplayEXT" ) ) );
        if ( GetPlatformDisplay != nullptr )
        {
            Display = GetPlatformDisplay( EglPlatformSurfacelessMesa, nullptr, nullptr );
        }
        if ( Display == 0U )
        {
            Display = GetDisplay( 0U );
        }
        if ( GCachedEglDisplay != 0U )
        {
            Display = GCachedEglDisplay;
        }
    }
    /* Ownership starts here: an eglInitialize failure still leaves a display that must be terminated. */
    bDisplayInitialized = bInitializeDisplay;
    if ( Display == 0U or ( bInitializeDisplay and InitializeDisplay( Display, nullptr, nullptr ) == EglFalse ) or
         BindAPI( EglOpenGLApi ) == EglFalse )
    {
        const EGLenum Error = ( GetErrorProc != nullptr ) ? GetErrorProc() : 0U;
        ( void )Error;
        AbortInitialize();
        return EglFailure( "Unable to initialize the EGL display for the OpenGL API." );
    }

    const EGLint SurfaceBits        = NativeWindow ? ( EglWindowBit | EglPbufferBit ) : EglPbufferBit;
    const EGLint ConfigAttributes[] = { EglSurfaceType, SurfaceBits, EglRenderableType, EglOpenGLBit,
                                        EglRedSize,     8,           EglGreenSize,      8,
                                        EglBlueSize,    8,           EglAlphaSize,      8,
                                        EglNone };
    EGLint ConfigCount              = 0;
    if ( ChooseConfig( Display, ConfigAttributes, &Config, 1, &ConfigCount ) == EglFalse or ConfigCount < 1 )
    {
        AbortInitialize();
        return EglFailure( "EGL could not provide an RGBA8/depth24 OpenGL config." );
    }

    EGLint ContextAttributes[7] = {
        EglContextMajorVersion,         4,      EglContextMinorVersion, 6, EglContextOpenGLProfileMask,
        EglContextOpenGLCoreProfileBit, EglNone };
    if ( bEnableDebug )
    {
        ContextAttributes[5] = EglContextOpenGLCoreProfileBit | EglContextOpenGLDebugBit;
    }

    Context = CreateContextProc( Display, Config, 0U, ContextAttributes );
    if ( Context == 0U )
    {
        /**
         * INFO: Mesa's surfaceless EGL rejects EGL_CONTEXT_OPENGL_PROFILE_MASK even though
         * version-only 4.6 requests still yield a genuine Core Profile context.
         */
        ( void )GetErrorProc();
        const EGLint FallbackAttributes[] = { EglContextMajorVersion, 4, EglContextMinorVersion, 6, EglNone };
        Context                           = CreateContextProc( Display, Config, 0U, FallbackAttributes );
    }
    if ( Context == 0U )
    {
        AbortInitialize();
        return EglFailure( "EGL refused an OpenGL 4.6 core context; no 4.5 fallback is permitted." );
    }

    if ( MakeCurrentProc( Display, 0U, 0U, Context ) == EglFalse )
    {
        if ( CreatePbuffer == nullptr )
        {
            AbortInitialize();
            return EglFailure( "EGL surfaceless context is unavailable and no Pbuffer provider exists." );
        }
        const EGLint SurfaceAttributes[] = { EglWidth, 1, EglHeight, 1, EglNone };
        Surface                          = CreatePbuffer( Display, Config, SurfaceAttributes );
        if ( Surface == 0U or MakeCurrentProc( Display, Surface, Surface, Context ) == EglFalse )
        {
            AbortInitialize();
            return EglFailure( "Unable to make the EGL OpenGL context current." );
        }
    }

    if ( NativeWindow )
    {
        GCachedNativeEglDisplays[NativeWindow->NativeDisplay] = Display;
    }
    else
    {
        GCachedEglDisplay = Display;
    }
    ++GCachedDisplayUsers[Display];
    bDisplayInitialized     = false;
    bCachedDisplayReference = true;
    return {};
}

void Lumen::RHIOpenGL::FEGLPlatformContext::ReleaseEglObjects () noexcept
{
    if ( Display != 0U and MakeCurrentProc != nullptr )
    {
        ( void )MakeCurrentProc( Display, 0U, 0U, 0U );
    }
    if ( Display != 0U and Surface != 0U and DestroySurfaceProc != nullptr )
    {
        ( void )DestroySurfaceProc( Display, Surface );
    }
    Surface = 0U;
    if ( Display != 0U and Context != 0U and DestroyContextProc != nullptr )
    {
        ( void )DestroyContextProc( Display, Context );
    }
    Context = 0U;
    Config  = 0U;
}

void Lumen::RHIOpenGL::FEGLPlatformContext::AbortInitialize () noexcept
{
    /* Initialize() holds GEglCacheMutex: no cached reference can be taken on this path. */
    ReleaseEglObjects();
    if ( bDisplayInitialized and Display != 0U and TerminateDisplay != nullptr )
    {
        ( void )TerminateDisplay( Display );
    }
    bDisplayInitialized     = false;
    bCachedDisplayReference = false;
    Display                 = 0U;
    Library                 = nullptr;
    GetProc                 = nullptr;
    NativeWindow.Reset();
}

void Lumen::RHIOpenGL::FEGLPlatformContext::Shutdown () noexcept
{
    ReleaseEglObjects();

    if ( Display != 0U and TerminateDisplay != nullptr and ( bDisplayInitialized or bCachedDisplayReference ) )
    {
        Lumen::HAL::TLockGuard CacheLock( GEglCacheMutex );
        Bool bTerminate = bDisplayInitialized;
        if ( bCachedDisplayReference )
        {
            const auto CachedUser = GCachedDisplayUsers.find( Display );
            if ( CachedUser == GCachedDisplayUsers.end() or CachedUser->second == 1U )
            {
                if ( CachedUser != GCachedDisplayUsers.end() )
                {
                    GCachedDisplayUsers.erase( CachedUser );
                }
                bTerminate = true;
                if ( GCachedEglDisplay == Display )
                {
                    GCachedEglDisplay = 0U;
                }
                for ( auto CachedDisplay = GCachedNativeEglDisplays.begin(); CachedDisplay != GCachedNativeEglDisplays.end(); )
                {
                    if ( CachedDisplay->second == Display )
                    {
                        CachedDisplay = GCachedNativeEglDisplays.erase( CachedDisplay );
                    }
                    else
                    {
                        ++CachedDisplay;
                    }
                }
            }
            else
            {
                --CachedUser->second;
            }
            bCachedDisplayReference = false;
        }
        /* Terminate under the cache lock: a concurrent Initialize must not reuse a dead display. */
        if ( bTerminate )
        {
            ( void )TerminateDisplay( Display );
        }
    }

    bDisplayInitialized = false;
    Display             = 0U;
    Library             = nullptr;
    GetProc             = nullptr;
    NativeWindow.Reset();
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHIOpenGL::FEGLPlatformContext::MakeCurrent () noexcept
{
    if ( Display == 0U or Context == 0U or MakeCurrentProc == nullptr )
    {
        return EglFailure( "EGL context is not initialized." );
    }
    if ( MakeCurrentProc( Display, Surface, Surface, Context ) == EglFalse )
    {
        return EglFailure( "Unable to make the EGL OpenGL context current." );
    }
    return {};
}

Lumen::TExpected<Lumen::UPtr, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FEGLPlatformContext::CreateWindowSurface () noexcept
{
    if ( not NativeWindow or Display == 0U or Config == 0U or CreateWindow == nullptr )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Presentation,
                                                      "EGL was not initialized with a native window descriptor." ) );
    }
    const EGLNativeDisplayType NativeHandle = NativeWindow->WindowSystem == EOpenGLNativeWindowSystem::Wayland
                                                  ? NativeWindow->NativeWindowAttachment
                                                  : NativeWindow->NativeWindow;
    EGLSurface WindowSurface                = CreateWindow( Display, Config, NativeHandle, nullptr );
    if ( WindowSurface == 0U )
    {
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Presentation,
                                                        "eglCreateWindowSurface failed for the native OpenGL window." ) );
    }
    return WindowSurface;
}

void Lumen::RHIOpenGL::FEGLPlatformContext::DestroyWindowSurface ( const UPtr InSurface ) noexcept
{
    if ( Display != 0U and InSurface != 0U and DestroySurfaceProc != nullptr )
    {
        ( void )DestroySurfaceProc( Display, InSurface );
    }
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FEGLPlatformContext::MakeSurfaceCurrent ( const UPtr InSurface ) noexcept
{
    if ( Display == 0U or Context == 0U or InSurface == 0U or MakeCurrentProc == nullptr )
    {
        return EglFailure( "EGL window surface or context is not initialized." );
    }
    if ( MakeCurrentProc( Display, InSurface, InSurface, Context ) == EglFalse )
    {
        return EglFailure( "Unable to make the native EGL window surface current." );
    }
    return {};
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FEGLPlatformContext::SwapBuffers ( const UPtr InSurface, const Int32 InSwapInterval ) noexcept
{
    if ( SwapIntervalProc == nullptr or SwapBuffersProc == nullptr )
    {
        return EglFailure( "EGL native presentation entry points are unavailable." );
    }
    LUMEN_TRY_EXPECT_ERROR( MakeSurfaceCurrent( InSurface ) );
    if ( SwapIntervalProc( Display, InSwapInterval ) == EglFalse )
    {
        return EglFailure( "eglSwapInterval rejected the requested presentation mode." );
    }
    const EGLBoolean Swapped                       = SwapBuffersProc( Display, InSurface );
    const TExpected<void, RHI::FRHIError> Restored = MakeCurrent();
    if ( Swapped == EglFalse )
    {
        return EglFailure( "eglSwapBuffers failed for the native OpenGL window." );
    }
    return Restored;
}

Lumen::TOptional<Lumen::UPtr> Lumen::RHIOpenGL::FEGLPlatformContext::ResolveProcedure ( const FStringView InName ) const noexcept
{
    const FString Name( InName );
    UPtr Address = ( GetProc != nullptr ) ? reinterpret_cast<UPtr>( GetProc( Name.c_str() ) ) : 0U;

    if ( Address == 0U and Library != nullptr )
    {
        Address = LoadEglSymbol( Library, Name.c_str() );
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

void Lumen::RHIOpenGL::FEGLPlatformContext::InitializeEglProcedure () noexcept
{
    GetProc            = CastEglProcedure<TEGLGetProcAddress>( LoadEglSymbol( Library, "eglGetProcAddress" ) );
    GetDisplay         = CastEglProcedure<TEGLGetDisplay>( LoadEglSymbol( Library, "eglGetDisplay" ) );
    InitializeDisplay  = CastEglProcedure<TEGLInitialize>( LoadEglSymbol( Library, "eglInitialize" ) );
    TerminateDisplay   = CastEglProcedure<TEGLTerminate>( LoadEglSymbol( Library, "eglTerminate" ) );
    BindAPI            = CastEglProcedure<TEGLBindAPI>( LoadEglSymbol( Library, "eglBindAPI" ) );
    ChooseConfig       = CastEglProcedure<TEGLChooseConfig>( LoadEglSymbol( Library, "eglChooseConfig" ) );
    CreateContextProc  = CastEglProcedure<TEGLCreateContext>( LoadEglSymbol( Library, "eglCreateContext" ) );
    DestroyContextProc = CastEglProcedure<TEGLDestroyContext>( LoadEglSymbol( Library, "eglDestroyContext" ) );
    CreatePbuffer      = CastEglProcedure<TEGLCreatePbufferSurface>( LoadEglSymbol( Library, "eglCreatePbufferSurface" ) );
    CreateWindow       = CastEglProcedure<TEGLCreateWindowSurface>( LoadEglSymbol( Library, "eglCreateWindowSurface" ) );
    DestroySurfaceProc = CastEglProcedure<TEGLDestroySurface>( LoadEglSymbol( Library, "eglDestroySurface" ) );
    MakeCurrentProc    = CastEglProcedure<TEGLMakeCurrent>( LoadEglSymbol( Library, "eglMakeCurrent" ) );
    SwapIntervalProc   = CastEglProcedure<TEGLSwapInterval>( LoadEglSymbol( Library, "eglSwapInterval" ) );
    SwapBuffersProc    = CastEglProcedure<TEGLSwapBuffers>( LoadEglSymbol( Library, "eglSwapBuffers" ) );
    QueryStringProc    = CastEglProcedure<TEGLQueryString>( LoadEglSymbol( Library, "eglQueryString" ) );
    GetErrorProc       = CastEglProcedure<TEGLGetError>( LoadEglSymbol( Library, "eglGetError" ) );
}

Lumen::Bool Lumen::RHIOpenGL::FEGLPlatformContext::AreEglProcedureInvalid () const noexcept
{
    return GetProc == nullptr or GetDisplay == nullptr or InitializeDisplay == nullptr or TerminateDisplay == nullptr or
           BindAPI == nullptr or ChooseConfig == nullptr or CreateContextProc == nullptr or DestroyContextProc == nullptr or
           MakeCurrentProc == nullptr or QueryStringProc == nullptr or GetErrorProc == nullptr;
}

#endif
