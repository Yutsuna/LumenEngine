/**
 * @file OpenGLDeviceContext.cpp
 * @brief Implements shared OpenGL device execution state.
 */

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>

#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Loader/GLSpec.hpp>

#include <String/StringView.hpp>

/**
 * File-local helpers
 */

namespace
{

/** @brief Raises Status to Lost without ever downgrading a clean Closed state. */
void RaiseStatusToLost ( Lumen::TAtomic<Lumen::RHI::ERHIDeviceStatus> &InStatus ) noexcept
{
    Lumen::RHI::ERHIDeviceStatus Current = InStatus.load();
    while ( static_cast<Lumen::UInt8>( Lumen::RHI::ERHIDeviceStatus::Lost ) > static_cast<Lumen::UInt8>( Current ) and
            not InStatus.compare_exchange_weak( Current, Lumen::RHI::ERHIDeviceStatus::Lost ) )
    {
        /* A concurrent transition only ever raises the status; refresh Current and retry. */
    }
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLDeviceContext>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDeviceContext::Create ( const RHI::FRHIAdapterInfo &InAdapter,
                                                 const Bool bEnableDebug,
                                                 RHI::IRHIDebugSink *InDebugSink,
                                                 const FOpenGLNativeWindowDesc *InNativeWindow ) noexcept
{
    LUMEN_EXPECT( Context, FOpenGLContext::Create( bEnableDebug, InNativeWindow ) );
    Context->SetDebugSink( InDebugSink );

    FOpenGLNativeLimits NativeLimits;
    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    LUMEN_TRY_EXPECT_ERROR( Context->ExecuteOnGLThread( [&Dispatch, &NativeLimits] () noexcept
    {
        Dispatch.GetIntegerv( GlMaxUniformBufferBindings, &NativeLimits.MaxUniformBufferBindings );
        Dispatch.GetIntegerv( GlMaxShaderStorageBufferBindings, &NativeLimits.MaxShaderStorageBufferBindings );
        Dispatch.GetIntegerv( GlMaxCombinedTextureImageUnits, &NativeLimits.MaxCombinedTextureImageUnits );
        Dispatch.GetIntegerv( GlMaxImageUnits, &NativeLimits.MaxImageUnits );
    } ) );
    return MakeShared<FOpenGLDeviceContext>( static_cast<TSharedRef<FOpenGLContext> &&>( Context ), InAdapter.Capabilities,
                                             InAdapter.Limits, NativeLimits );
}

Lumen::RHIOpenGL::FOpenGLDeviceContext::FOpenGLDeviceContext ( TSharedRef<FOpenGLContext> InContext,
                                                               RHI::FRHICapabilities InCapabilities,
                                                               RHI::FRHILimits InLimits,
                                                               FOpenGLNativeLimits InNativeLimits ) noexcept
    : Context( static_cast<TSharedRef<FOpenGLContext> &&>( InContext ) ),
      Capabilities( static_cast<RHI::FRHICapabilities &&>( InCapabilities ) ), Limits( InLimits ), NativeLimits( InNativeLimits )
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

Lumen::RHIOpenGL::FOpenGLWaitState *Lumen::RHIOpenGL::FOpenGLDeviceContext::GetWaitState () const noexcept
{
    return Context->GetWaitState();
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLDeviceContext::IsUnavailable () const noexcept
{
    return Status.load() != RHI::ERHIDeviceStatus::Ready or Context->IsDeviceLost();
}

const Lumen::RHI::FRHICapabilities &Lumen::RHIOpenGL::FOpenGLDeviceContext::GetCapabilities () const noexcept
{
    return Capabilities;
}

const Lumen::RHI::FRHILimits &Lumen::RHIOpenGL::FOpenGLDeviceContext::GetLimits () const noexcept
{
    return Limits;
}

const Lumen::RHIOpenGL::FOpenGLNativeLimits &Lumen::RHIOpenGL::FOpenGLDeviceContext::GetNativeLimits () const noexcept
{
    return NativeLimits;
}

Lumen::TOptional<Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDeviceContext::GetUnavailableError ( const FStringView InOperation ) const noexcept
{
    if ( Context->IsDeviceLost() )
    {
        RaiseStatusToLost( Status );
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
    RaiseStatusToLost( Status );
    Context->MarkDeviceLost();
}

void Lumen::RHIOpenGL::FOpenGLDeviceContext::MarkClosed () noexcept
{
    RHI::ERHIDeviceStatus Expected = RHI::ERHIDeviceStatus::Ready;
    if ( Status.compare_exchange_strong( Expected, RHI::ERHIDeviceStatus::Closed ) )
    {
        /* Retire in-flight work before waking host waiters: OnFailure marks the command
         * lists complete and publishes queue completions (D7 ordering). This is a blocking
         * round-trip to the executor and must never be called under a ranked lock. */
        Context->MarkDeviceClosed();
    }
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLDeviceContext::IsDeviceLost () const noexcept
{
    return Context->IsDeviceLost();
}

void Lumen::RHIOpenGL::FOpenGLDeviceContext::DetachDebugSink () noexcept
{
    Context->SetDebugSink( nullptr );
}
