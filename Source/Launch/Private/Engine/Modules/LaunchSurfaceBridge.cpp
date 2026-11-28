/**
 * @file LaunchSurfaceBridge.cpp
 * @brief Converts portable native window handles into an OpenGL surface bridge request.
 */

#include <Engine/Modules/LaunchSurfaceBridge.hpp>

#include <Templates/Unexpected.hpp>

/**
 * Public
 */

Lumen::TOptional<Lumen::RHIOpenGL::FOpenGLNativeWindowDesc>
Lumen::Launch::MakeOpenGLNativeWindowDesc ( const FNativeWindowHandles &InHandles, const RHI::FRHIExtent3D &InExtent ) noexcept
{
    RHIOpenGL::EOpenGLNativeWindowSystem WindowSystem = RHIOpenGL::EOpenGLNativeWindowSystem::Unknown;

    switch ( InHandles.WindowSystem )
    {
    case ENativeWindowSystem::X11:
        WindowSystem = RHIOpenGL::EOpenGLNativeWindowSystem::X11;
        break;
    case ENativeWindowSystem::Wayland:
        WindowSystem = RHIOpenGL::EOpenGLNativeWindowSystem::Wayland;
        break;
    case ENativeWindowSystem::Win32:
        WindowSystem = RHIOpenGL::EOpenGLNativeWindowSystem::Win32;
        break;
    case ENativeWindowSystem::Unknown:
    default:
        return {};
    }

    /** INFO: Win32 derives its device context from the window handle, so NativeDisplay may be zero. */
    if ( InHandles.NativeWindow == 0U or
         ( WindowSystem != RHIOpenGL::EOpenGLNativeWindowSystem::Win32 and InHandles.NativeDisplay == 0U ) )
    {
        return {};
    }

    RHIOpenGL::FOpenGLNativeWindowDesc Desc;
    Desc.WindowSystem           = WindowSystem;
    Desc.NativeWindow           = InHandles.NativeWindow;
    Desc.NativeDisplay          = InHandles.NativeDisplay;
    Desc.NativeWindowAttachment = InHandles.NativeWindowAttachment;
    Desc.Extent                 = InExtent;
    return TOptional<RHIOpenGL::FOpenGLNativeWindowDesc>( Desc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISurfaceBridge>, Lumen::RHI::FRHIError>
Lumen::Launch::CreateSurfaceBridge ( const FNativeWindowHandles &InHandles, const RHI::FRHIExtent3D &InExtent ) noexcept
{
    const auto CreateBridge = [] ( const RHIOpenGL::FOpenGLNativeWindowDesc &InDesc ) noexcept
    {
        return RHIOpenGL::CreateRHIOpenGLSurfaceBridge( InDesc );
    };

    return MakeOpenGLNativeWindowDesc( InHandles, InExtent )
        .Transform( CreateBridge )
        .ValueOr( TUnexpected( RHI::MakeRHIInvalidPresentation( "Portable native window handles are unavailable." ) ) );
}
