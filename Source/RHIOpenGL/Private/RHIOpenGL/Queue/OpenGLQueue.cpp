/**
 * @file OpenGLQueue.cpp
 * @brief Implements serialized submission with non-blocking GL fence completion.
 */

#include <RHIOpenGL/Queue/OpenGLQueue.hpp>

#include <Algo/ForEach.hpp>

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformTime.hpp>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Commands/OpenGLCommandExecutor.hpp>
#include <RHIOpenGL/Commands/OpenGLCommandList.hpp>
#include <RHIOpenGL/Synchronization/OpenGLSemaphore.hpp>

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
    return RHI::FRHITimelineValue{ CompletedValue->load() };
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLQueue::Submit ( const RHI::FRHISubmitDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Queue::Submit" ) );
    LUMEN_EXPECT( Validated, InDesc.Validate() );

    HAL::TLockGuard<HAL::FPlatformMutex> Lock( SubmitMutex );
    if ( InDesc.Completion.Value <= SubmittedValue.load() )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Queue, "Queue completion values must increase." ) );
    }

    if ( not Executor.IsValid() )
    {
        const TExpected<TSharedRef<FOpenGLCommandExecutor>, RHI::FRHIError> Created = FOpenGLCommandExecutor::Create( Context );
        LUMEN_TRY_EXPECT_ERROR( Created );
        Executor = *Created;
    }

    TVector<TSharedRef<RHI::IRHICommandList>> Lists;
    Lists.reserve( InDesc.CommandLists.size() );
    for ( const TSharedRef<RHI::IRHICommandList> &CommandList : InDesc.CommandLists )
    {
        const TOptional<FOpenGLCommandList *> Native = RHI::TryRHIBackendCast<FOpenGLCommandList>( *CommandList );
        if ( not Native )
        {
            return TUnexpected(
                RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Queue, "Submission contains a foreign command list." ) );
        }
        if ( ( *Native )->GetQueueType() != QueueType )
        {
            return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Queue,
                                                             "Command list queue type does not match the queue." ) );
        }
        Lists.push_back( CommandList );
    }

    FOpenGLSubmission Packet;
    TVector<TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64>> BinaryWaits;
    for ( const RHI::FRHISemaphoreOperation &Operation : InDesc.Waits )
    {
        const TOptional<FOpenGLSemaphore *> Native =
            RHI::TryRHIBackendCast<FOpenGLSemaphore>( static_cast<RHI::IRHIObject &>( *Operation.Semaphore ) );
        if ( not Native )
        {
            return TUnexpected(
                RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Queue, "Submission contains a foreign wait semaphore." ) );
        }
        if ( ( *Native )->GetValue() < Operation.Value )
        {
            return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Queue, "Submission wait is not signaled." ) );
        }
        TSharedPtr<RHI::IRHISemaphore> Retained = Operation.Semaphore;
        const Bool bBinaryWait                  = Retained->GetDesc().Type == RHI::ERHISemaphoreType::Binary;
        if ( bBinaryWait )
        {
            BinaryWaits.push_back( TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64>( Retained, Operation.Value ) );
        }
        else
        {
            Packet.Waits.push_back( TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64>( Retained, Operation.Value ) );
        }
    }

    for ( const RHI::FRHISemaphoreOperation &Operation : InDesc.Signals )
    {
        const TOptional<FOpenGLSemaphore *> Native =
            RHI::TryRHIBackendCast<FOpenGLSemaphore>( static_cast<RHI::IRHIObject &>( *Operation.Semaphore ) );
        if ( not Native )
        {
            return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Queue,
                                                             "Submission contains a foreign signal semaphore." ) );
        }
        Packet.Signals.push_back( TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64>(
            TSharedPtr<RHI::IRHISemaphore>( Operation.Semaphore ), Operation.Value ) );
    }

    /* Binary waits consume the semaphore, so only do so after every wait and signal has been validated. */
    for ( const TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64> &BinaryWait : BinaryWaits )
    {
        LUMEN_TRY_EXPECT_ERROR( BinaryWait.first->Wait( BinaryWait.second, 0.0 ) );
    }

    USize MarkedCount = 0U;
    for ( const TSharedRef<RHI::IRHICommandList> &CommandList : Lists )
    {
        const TOptional<FOpenGLCommandList *> Native = RHI::TryRHIBackendCast<FOpenGLCommandList>( *CommandList );
        const RHI::FRHIExpected Pending              = ( *Native )->MarkPending( InDesc.Completion );
        if ( not Pending )
        {
            /* Earlier lists are already Pending; release them so they can be recorded again. */
            for ( USize Index = 0U; Index < MarkedCount; ++Index )
            {
                const TOptional<FOpenGLCommandList *> Marked = RHI::TryRHIBackendCast<FOpenGLCommandList>( *Lists[Index] );
                ( void )( *Marked )->MarkCompleted( InDesc.Completion );
            }
            return TUnexpected<RHI::FRHIError>( Pending.Error() );
        }
        ++MarkedCount;
    }

    const TSharedRef<TAtomic<UInt64>> RetiredValue            = CompletedValue;
    const TSharedPtr<FOpenGLCommandExecutor> RetainedExecutor = Executor;
    Packet.Completion                                         = InDesc.Completion;
    Packet.Replay                                             = [RetainedExecutor, Lists] () noexcept -> RHI::FRHIExpected
    {
        RHI::FRHIExpected Result;
        for ( const TSharedRef<RHI::IRHICommandList> &CommandList : Lists )
        {
            const TOptional<FOpenGLCommandList *> Native = RHI::TryRHIBackendCast<FOpenGLCommandList>( *CommandList );
            const RHI::FRHIExpected Replayed             = RetainedExecutor->Execute( ( *Native )->GetStream() );
            if ( not Replayed and Result )
            {
                Result = Replayed;
            }
        }
        return Result;
    };
    Packet.OnComplete = [RetiredValue, Lists, InDesc] () noexcept
    {
        UInt64 Observed = RetiredValue->load();
        while ( Observed < InDesc.Completion.Value and
                not RetiredValue->compare_exchange_weak( Observed, InDesc.Completion.Value ) )
        {
            /* A failed exchange refreshes Observed with the current value. */
        }
        Algo::ForEach( Lists,
                       [] ( const TSharedRef<RHI::IRHICommandList> &InCommandList ) noexcept -> Bool
        {
            return RHI::TryRHIBackendCast<FOpenGLCommandList>( *InCommandList ).HasValue();
        }, [&InDesc] ( const TSharedRef<RHI::IRHICommandList> &InCommandList ) noexcept
        {
            const TOptional<FOpenGLCommandList *> Native = RHI::TryRHIBackendCast<FOpenGLCommandList>( *InCommandList );
            ( void )( *Native )->MarkCompleted( InDesc.Completion );
        } );
    };
    Packet.OnFailure = [Lists, InDesc] ( RHI::FRHIError ) noexcept
    {
        Algo::ForEach( Lists,
                       [] ( const TSharedRef<RHI::IRHICommandList> &InCommandList ) noexcept -> Bool
        {
            return RHI::TryRHIBackendCast<FOpenGLCommandList>( *InCommandList ).HasValue();
        }, [&InDesc] ( const TSharedRef<RHI::IRHICommandList> &InCommandList ) noexcept
        {
            const TOptional<FOpenGLCommandList *> Native = RHI::TryRHIBackendCast<FOpenGLCommandList>( *InCommandList );
            ( void )( *Native )->MarkCompleted( InDesc.Completion );
        } );
    };

    LUMEN_TRY_EXPECT_ERROR( Context->GetExecutor()->Submit( static_cast<FOpenGLSubmission &&>( Packet ) ) );
    UInt64 Submitted = SubmittedValue.load();
    while ( Submitted < InDesc.Completion.Value and
            not SubmittedValue.compare_exchange_weak( Submitted, InDesc.Completion.Value ) )
    {
        /* A failed exchange refreshes Submitted with the current value. */
    }
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLQueue::WaitIdle () noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Queue::WaitIdle" ) );
    while ( CompletedValue->load() < SubmittedValue.load() )
    {
        LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Queue::WaitIdle" ) );
        HAL::FPlatformTime::Sleep( 0.0005 );
    }
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

    if ( not InValue.IsValid() or InValue.Value <= CompletedValue->load() )
    {
        return {};
    }
    const Float64 Deadline = HAL::FPlatformTime::Seconds() + InTimeoutSeconds;
    while ( InValue.Value > CompletedValue->load() )
    {
        LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Queue::Wait" ) );
        if ( HAL::FPlatformTime::Seconds() >= Deadline )
        {
            break;
        }
        HAL::FPlatformTime::Sleep( 0.0005 );
    }
    if ( InValue.Value > CompletedValue->load() )
    {
        return TUnexpected(
            RHI::MakeRHITimeout( RHI::ERHIErrorCondition::Synchronization, "Requested timeline value has not completed." ) );
    }
    if ( Context->GetUnavailableError( "Queue::Wait" ) )
    {
        return TUnexpected(
            RHI::MakeRHIDeviceLost( RHI::ERHIErrorCondition::Synchronization, "OpenGL device was lost during a queue wait." ) );
    }
    return {};
}
