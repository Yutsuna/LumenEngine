/**
 * @file OpenGLSurfaceBridge.cpp
 * @brief Validates native-window handles at the OpenGL surface factory boundary.
 */

#include <RHIOpenGL/RHIOpenGLSurfaceBridge.hpp>

#include <Templates/Unexpected.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISurfaceBridge>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::CreateRHIOpenGLSurfaceBridge ( const FOpenGLNativeWindowDesc &InDesc ) noexcept
{
    if ( ( InDesc.WindowSystem != EOpenGLNativeWindowSystem::X11 and
           InDesc.WindowSystem != EOpenGLNativeWindowSystem::Wayland ) or
         InDesc.NativeWindow == 0U or InDesc.NativeDisplay == 0U or not InDesc.Extent.IsValid() or InDesc.Extent.Depth != 1U )
    {
        return TUnexpected( RHI::MakeRHIInvalidPresentation( "Native OpenGL window handles and extent are required." ) );
    }

    if ( InDesc.WindowSystem == EOpenGLNativeWindowSystem::Wayland and InDesc.NativeWindowAttachment == 0U )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Presentation,
                                                     "Wayland OpenGL presentation requires a native EGL window attachment." ) );
    }

    return TUnexpected( RHI::MakeRHIUnsupported(
        RHI::ERHIErrorCondition::Presentation,
        "Windowed OpenGL requires device initialization on the native EGLDisplay; the current device uses a surfaceless "
        "context that cannot be shared across EGLDisplays." ) );
}
