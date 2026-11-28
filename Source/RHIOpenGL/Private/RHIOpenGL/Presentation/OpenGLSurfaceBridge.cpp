/**
 * @file OpenGLSurfaceBridge.cpp
 * @brief Implements native EGL surface creation, context selection and presentation.
 */

#include <RHIOpenGL/Presentation/OpenGLSurfaceBridge.hpp>

#include <Templates/Unexpected.hpp>

/**
 * Public
 */

Lumen::RHIOpenGL::FOpenGLSurfaceBridge::FOpenGLSurfaceBridge ( const FOpenGLNativeWindowDesc &InDesc ) noexcept
    : Desc( InDesc ), Width( InDesc.Extent.Width ), Height( InDesc.Extent.Height )
{
    /* */
}

void Lumen::RHIOpenGL::FOpenGLSurfaceBridge::AttachContext ( TSharedRef<FOpenGLContext> InContext ) noexcept
{
    Context = InContext;
}

void Lumen::RHIOpenGL::FOpenGLSurfaceBridge::SetExtent ( const RHI::FRHIExtent3D &InExtent ) noexcept
{
    Width.store( InExtent.Width );
    Height.store( InExtent.Height );
}

Lumen::TExpected<Lumen::RHI::FRHINativeSurface, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLSurfaceBridge::CreateNativeSurface ( const UPtr InBackendHandle ) noexcept
{
    if ( not Context.IsValid() )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Presentation,
                                                      "The OpenGL surface bridge is not attached to a device context." ) );
    }

    TExpected<RHI::FRHINativeSurface, RHI::FRHIError> Result =
        TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Presentation, "EGL surface creation did not run." ) );
    LUMEN_TRY_EXPECT_ERROR( Context->ExecuteOnGLThread( [this, InBackendHandle, &Result] () noexcept
    {
        const TExpected<EGLSurface, RHI::FRHIError> Surface = Context->GetNativeContext().CreateWindowSurface();
        if ( not Surface )
        {
            Result = TUnexpected( Surface.Error() );
            return;
        }
        Result = RHI::FRHINativeSurface{ .BackendHandle = InBackendHandle, .SurfaceHandle = *Surface, .ContextHandle = 0U };
    } ) );
    return Result;
}

void Lumen::RHIOpenGL::FOpenGLSurfaceBridge::DestroyNativeSurface ( const RHI::FRHINativeSurface &InSurface ) noexcept
{
    if ( Context.IsValid() and InSurface.SurfaceHandle != 0U )
    {
        ( void )Context->ExecuteOnGLThread( [this, InSurface] () noexcept
        {
            Context->GetNativeContext().DestroyWindowSurface( InSurface.SurfaceHandle );
        } );
    }
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLSurfaceBridge::MakeCurrent ( const RHI::FRHINativeSurface &InSurface ) noexcept
{
    if ( not Context.IsValid() or InSurface.SurfaceHandle == 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidPresentation( "The native OpenGL surface is not initialized." ) );
    }
    RHI::FRHIExpected Result{};
    LUMEN_TRY_EXPECT_ERROR( Context->ExecuteOnGLThread( [this, &InSurface, &Result] () noexcept
    {
        Result = Context->GetNativeContext().MakeSurfaceCurrent( InSurface.SurfaceHandle );
    } ) );
    return Result;
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLSurfaceBridge::Present ( const RHI::FRHINativeSurface &InSurface,
                                                                           const Int32 InSwapInterval ) noexcept
{
    if ( not Context.IsValid() or InSurface.SurfaceHandle == 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidPresentation( "The native OpenGL surface is not initialized." ) );
    }
    RHI::FRHIExpected Result{};
    LUMEN_TRY_EXPECT_ERROR( Context->ExecuteOnGLThread( [this, &InSurface, InSwapInterval, &Result] () noexcept
    {
        Result = Context->GetNativeContext().SwapBuffers( InSurface.SurfaceHandle, InSwapInterval );
    } ) );
    return Result;
}

Lumen::TExpected<Lumen::RHI::FRHIExtent3D, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLSurfaceBridge::GetExtent () const noexcept
{
    return RHI::FRHIExtent3D{ Width.load(), Height.load(), 1U };
}

Lumen::TOptional<Lumen::UPtr> Lumen::RHIOpenGL::FOpenGLSurfaceBridge::ResolveProcedure ( const FStringView InName ) noexcept
{
    if ( not Context.IsValid() )
    {
        return {};
    }
    TOptional<UPtr> Result;
    if ( not Context->ExecuteOnGLThread( [this, InName, &Result] () noexcept
    {
        Result = Context->GetNativeContext().ResolveProcedure( InName );
    } ) )
    {
        return {};
    }
    return Result;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISurfaceBridge>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::CreateRHIOpenGLSurfaceBridge ( const FOpenGLNativeWindowDesc &InDesc ) noexcept
{
    const Bool bKnownSystem = InDesc.WindowSystem == EOpenGLNativeWindowSystem::X11 or
                              InDesc.WindowSystem == EOpenGLNativeWindowSystem::Wayland or
                              InDesc.WindowSystem == EOpenGLNativeWindowSystem::Win32;
    /* Win32 derives its device context from the window handle, so NativeDisplay is not required. */
    const Bool bHasDisplay = InDesc.WindowSystem == EOpenGLNativeWindowSystem::Win32 or InDesc.NativeDisplay != 0U;

    if ( not bKnownSystem or InDesc.NativeWindow == 0U or not bHasDisplay or not InDesc.Extent.IsValid() or
         InDesc.Extent.Depth != 1U )
    {
        return TUnexpected( RHI::MakeRHIInvalidPresentation( "Native OpenGL window handles and extent are required." ) );
    }

    if ( InDesc.WindowSystem == EOpenGLNativeWindowSystem::Wayland and InDesc.NativeWindowAttachment == 0U )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Presentation,
                                                     "Wayland OpenGL presentation requires a native EGL window attachment." ) );
    }

    return MakeShared<FOpenGLSurfaceBridge>( InDesc );
}
