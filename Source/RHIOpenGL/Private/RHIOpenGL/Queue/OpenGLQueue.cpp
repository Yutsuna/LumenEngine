/**
 * @file OpenGLQueue.cpp
 * @brief Implements synchronous OpenGL command replay and timeline waits.
 */

#include <RHIOpenGL/Queue/OpenGLQueue.hpp>

#include <HAL/LockGuard.hpp>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Commands/OpenGLCommandExecutor.hpp>
#include <RHIOpenGL/Commands/OpenGLCommandList.hpp>

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
    LUMEN_EXPECT( Validated, InDesc.Validate() );

    HAL::TLockGuard<HAL::FPlatformMutex> Lock( SubmitMutex );
    if ( not Executor.IsValid() )
    {
        const TExpected<TSharedRef<FOpenGLCommandExecutor>, RHI::FRHIError> Created = FOpenGLCommandExecutor::Create( Context );
        if ( not Created )
        {
            return TUnexpected( Created.Error() );
        }
        Executor = *Created;
    }

    RHI::FRHIExpected Result;
    for ( const TSharedRef<RHI::IRHICommandList> &CommandList : InDesc.CommandLists )
    {
        const TOptional<FOpenGLCommandList *> Native = RHI::TryRHIBackendCast<FOpenGLCommandList>( *CommandList );
        if ( not Native )
        {
            if ( Result )
            {
                Result = TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Queue,
                                                                   "Submission contains a foreign command list." ) );
            }
            continue;
        }
        FOpenGLCommandList *List = *Native;

        const RHI::FRHIExpected Pending = List->MarkPending( InDesc.Completion );
        if ( not Pending )
        {
            if ( Result )
            {
                Result = Pending;
            }
            ( void )List->MarkCompleted( InDesc.Completion );
            continue;
        }

        const RHI::FRHIExpected Replayed = Executor->Execute( List->GetStream() );
        if ( not Replayed and Result )
        {
            Result = Replayed;
        }
        const RHI::FRHIExpected Completed = List->MarkCompleted( InDesc.Completion );
        if ( not Completed and Result )
        {
            Result = Completed;
        }
    }

    UInt64 Observed = CompletedValue.load();
    while ( Observed < InDesc.Completion.Value and not CompletedValue.compare_exchange_weak( Observed, InDesc.Completion.Value ) )
    {
        /* The failed exchange refreshes Observed with the current value. */
    }
    return Result;
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
