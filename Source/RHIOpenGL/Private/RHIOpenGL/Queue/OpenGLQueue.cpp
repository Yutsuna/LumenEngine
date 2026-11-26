/**
 * @file OpenGLQueue.cpp
 * @brief Implements logical OpenGL queue queries and host waits.
 */

#include <RHIOpenGL/Queue/OpenGLQueue.hpp>

#include <RHI/Types/RHIError.hpp>

#include <Templates/Optional.hpp>

#include <cmath>

/**
 * Public
 */

Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLQueue>
Lumen::RHIOpenGL::FOpenGLQueue::Create ( const RHI::FRHIObjectId InId,
                                         TSharedRef<FOpenGLDeviceContext> InContext,
                                         const RHI::ERHIQueueType InType,
                                         const UInt32 InIndex ) noexcept
{
    return MakeShared<FOpenGLQueue>( InId, static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InType, InIndex );
}

Lumen::RHIOpenGL::FOpenGLQueue::FOpenGLQueue ( const RHI::FRHIObjectId InId,
                                               TSharedRef<FOpenGLDeviceContext> InContext,
                                               const RHI::ERHIQueueType InType,
                                               const UInt32 InIndex ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLQueue>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), QueueType( InType ), QueueIndex( InIndex )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLQueue )

Lumen::RHI::ERHIQueueType Lumen::RHIOpenGL::FOpenGLQueue::GetType () const noexcept
{
    return QueueType;
}

Lumen::RHI::FRHITimelineValue Lumen::RHIOpenGL::FOpenGLQueue::GetCompletedValue () const noexcept
{
    return RHI::FRHITimelineValue{ CompletedValue.load() };
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLQueue::Submit ( const RHI::FRHISubmitDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Queue::Submit" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );

    return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Queue,
                                                 "OpenGL queue submission is implemented in a later milestone." ) );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLQueue::WaitIdle () noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Queue::WaitIdle" ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLQueue::Wait ( const RHI::FRHITimelineValue InValue,
                                                                const Float64 InTimeoutSeconds ) noexcept
{
    if ( not std::isfinite( InTimeoutSeconds ) or InTimeoutSeconds < 0.0 )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Synchronization,
                                                         "Queue wait timeout must be finite and nonnegative." ) );
    }
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Queue::Wait" ) );

    if ( not InValue.IsValid() or RHI::IsRHICompletionReached( GetCompletedValue(), InValue ) )
    {
        return {};
    }
    return TUnexpected(
        RHI::MakeRHITimeout( RHI::ERHIErrorCondition::Synchronization, "Requested timeline value has not completed." ) );
}
