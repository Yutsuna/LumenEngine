/**
 * @file OpenGLSemaphore.cpp
 * @brief Implements CPU-visible binary and timeline semaphore semantics.
 */

#include <RHIOpenGL/Synchronization/OpenGLSemaphore.hpp>

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformTime.hpp>

#include <RHI/Types/RHIError.hpp>

#include <Templates/Optional.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLSemaphore>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLSemaphore::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                             const RHI::FRHISemaphoreDesc &InDesc,
                                             const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    LUMEN_TRY_OPTIONAL_ERROR( InContext->GetUnavailableError( "Semaphore::Create" ) );
    if ( not InContext->GetCapabilities().Supports( RHI::ERHICapabilityFlags::TimelineSemaphores ) and
         InDesc.Type == RHI::ERHISemaphoreType::Timeline )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Capability, "Timeline semaphores were not negotiated." ) );
    }
    return MakeShared<FOpenGLSemaphore>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
}

Lumen::RHIOpenGL::FOpenGLSemaphore::FOpenGLSemaphore ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                       const RHI::FRHISemaphoreDesc &InDesc,
                                                       const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLSemaphore>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc ), Value{ InDesc.InitialValue }
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLSemaphore )

const Lumen::RHI::FRHISemaphoreDesc &Lumen::RHIOpenGL::FOpenGLSemaphore::GetDesc () const noexcept
{
    return Desc;
}

Lumen::UInt64 Lumen::RHIOpenGL::FOpenGLSemaphore::GetValue () const noexcept
{
    return Value.load();
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLSemaphore::Signal ( const UInt64 InValue ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Semaphore::Signal" ) );
    {
        HAL::TLockGuard Lock( Mutex );
        if ( Desc.Type == RHI::ERHISemaphoreType::Binary )
        {
            if ( InValue != 1ULL )
            {
                return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Synchronization,
                                                              "Binary semaphores can only signal value one." ) );
            }
        }
        else if ( InValue < Value.load() )
        {
            return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Synchronization,
                                                          "Timeline semaphore signal would regress." ) );
        }
        Value.store( InValue );
    }
    Condition.NotifyAll();
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLSemaphore::Wait ( const UInt64 InValue,
                                                                    const Float64 InTimeoutSeconds ) noexcept
{
    if ( InTimeoutSeconds < 0.0 )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Synchronization,
                                                         "Semaphore wait timeout must be nonnegative." ) );
    }
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Semaphore::Wait" ) );
    if ( Desc.Type == RHI::ERHISemaphoreType::Binary and InValue != 1ULL )
    {
        return TUnexpected( RHI::MakeRHIInvalidSynchronization( "Binary semaphores can only wait on value one." ) );
    }

    const Float64 Deadline = HAL::FPlatformTime::Seconds() + InTimeoutSeconds;
    {
        HAL::TUniqueLock Lock( Mutex );
        const auto IsSignaled = [this, InValue] () noexcept -> Bool
        {
            return Value.load() >= InValue;
        };
        if ( InTimeoutSeconds <= 0.0 )
        {
            ( void )Condition.WaitFor( Lock, 0.0, IsSignaled );
        }
        else
        {
            ( void )Condition.WaitUntil( Lock, Deadline, IsSignaled );
        }
        const Bool bSignaled = IsSignaled();
        if ( bSignaled and Desc.Type == RHI::ERHISemaphoreType::Binary )
        {
            Value.store( 0ULL );
        }
        if ( not bSignaled )
        {
            return TUnexpected( RHI::MakeRHITimeout( RHI::ERHIErrorCondition::Synchronization, "Semaphore wait timed out." ) );
        }
    }
    if ( Context->GetUnavailableError( "Semaphore::Wait" ) )
    {
        return TUnexpected( RHI::MakeRHIDeviceLost( RHI::ERHIErrorCondition::Synchronization,
                                                    "OpenGL device was lost while waiting on a semaphore." ) );
    }
    return {};
}
