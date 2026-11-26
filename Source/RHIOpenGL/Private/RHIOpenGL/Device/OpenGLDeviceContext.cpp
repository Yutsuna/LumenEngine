/**
 * @file OpenGLDeviceContext.cpp
 * @brief Implements shared OpenGL device execution state.
 */

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>

#include <RHI/Types/RHIError.hpp>

#include <String/StringView.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLDeviceContext>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDeviceContext::Create ( const RHI::FRHIAdapterInfo &InAdapter,
                                                 const Bool bEnableDebug,
                                                 RHI::IRHIDebugSink *InDebugSink ) noexcept
{
    LUMEN_EXPECT( Context, FOpenGLContext::Create( bEnableDebug ) );
    Context->SetDebugSink( InDebugSink );
    return MakeShared<FOpenGLDeviceContext>( static_cast<TSharedRef<FOpenGLContext> &&>( Context ), InAdapter.Capabilities,
                                             InAdapter.Limits );
}

Lumen::RHIOpenGL::FOpenGLDeviceContext::FOpenGLDeviceContext ( TSharedRef<FOpenGLContext> InContext,
                                                               RHI::FRHICapabilities InCapabilities,
                                                               RHI::FRHILimits InLimits ) noexcept
    : Context( static_cast<TSharedRef<FOpenGLContext> &&>( InContext ) ),
      Capabilities( static_cast<RHI::FRHICapabilities &&>( InCapabilities ) ), Limits( InLimits )
{
    /* */
}

const Lumen::RHIOpenGL::FGLDispatchTable &Lumen::RHIOpenGL::FOpenGLDeviceContext::GetDispatch () const noexcept
{
    return Context->GetDispatch();
}

Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLContext> Lumen::RHIOpenGL::FOpenGLDeviceContext::GetExecutor () const noexcept
{
    return Context;
}

const Lumen::RHI::FRHICapabilities &Lumen::RHIOpenGL::FOpenGLDeviceContext::GetCapabilities () const noexcept
{
    return Capabilities;
}

const Lumen::RHI::FRHILimits &Lumen::RHIOpenGL::FOpenGLDeviceContext::GetLimits () const noexcept
{
    return Limits;
}

Lumen::TOptional<Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDeviceContext::GetUnavailableError ( const FStringView InOperation ) const noexcept
{
    if ( Context->IsDeviceLost() )
    {
        Status.store( RHI::ERHIDeviceStatus::Lost );
    }
    const RHI::ERHIDeviceStatus Current = Status.load();
    if ( Current == RHI::ERHIDeviceStatus::Ready )
    {
        return {};
    }

    const Bool bIsLost            = Current == RHI::ERHIDeviceStatus::Lost;
    const RHI::ERHIErrorCode Code = bIsLost ? RHI::ERHIErrorCode::DeviceLost : RHI::ERHIErrorCode::DeviceClosed;
    const RHI::FRHIError Error( Code, RHI::ERHIErrorCondition::Device, "OpenGL device is unavailable." );

    return TOptional<RHI::FRHIError>( InOperation.empty() ? Error : Error.WithContext( InOperation ) );
}

void Lumen::RHIOpenGL::FOpenGLDeviceContext::MarkLost () noexcept
{
    Status.store( RHI::ERHIDeviceStatus::Lost );
}

void Lumen::RHIOpenGL::FOpenGLDeviceContext::MarkClosed () noexcept
{
    RHI::ERHIDeviceStatus Expected = RHI::ERHIDeviceStatus::Ready;
    ( void )Status.compare_exchange_strong( Expected, RHI::ERHIDeviceStatus::Closed );
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLDeviceContext::IsDeviceLost () const noexcept
{
    return Context->IsDeviceLost();
}

void Lumen::RHIOpenGL::FOpenGLDeviceContext::DetachDebugSink () noexcept
{
    Context->SetDebugSink( nullptr );
}
