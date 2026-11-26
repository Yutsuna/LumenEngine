/**
 * @file OpenGLContext.cpp
 * @brief Implements the GL executor, fence-driven submission scheduler and debug bridge.
 */

#include <RHIOpenGL/Device/OpenGLContext.hpp>

#include <Algo/Any.hpp>
#include <Algo/ForEach.hpp>

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformTime.hpp>

#include <String/StringView.hpp>

#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Loader/GLLoader.hpp>
#include <RHIOpenGL/Synchronization/OpenGLSemaphore.hpp>

#include <Templates/Optional.hpp>

/**
 * File-local helpers
 */

namespace
{

struct FTaskLatch
{
    Lumen::HAL::FPlatformMutex Mutex;
    Lumen::FConditionVariable Done;
    Lumen::Bool bFinished = false;
};

/** @brief Kept out of line so both caller and worker own the latch until the task finishes. */
LUMEN_NO_INLINE Lumen::TSharedRef<FTaskLatch> MakeTaskLatch () noexcept
{
    return Lumen::MakeShared<FTaskLatch>();
}

/** @brief KHR_debug thunk: copies the native message; never re-enters GL or RHI. */
void DebugMessageThunk ( Lumen::RHIOpenGL::GLenum InSource,
                         Lumen::RHIOpenGL::GLenum InType,
                         Lumen::RHIOpenGL::GLuint InCode,
                         Lumen::RHIOpenGL::GLenum InSeverity,
                         Lumen::RHIOpenGL::GLsizei InLength,
                         const Lumen::AnsiChar *InText,
                         const void *InUserParam ) noexcept
{
    const Lumen::RHIOpenGL::FOpenGLContext *Context = static_cast<const Lumen::RHIOpenGL::FOpenGLContext *>( InUserParam );
    if ( Context != nullptr )
    {
        Context->NotifyDebugMessage( InSource, InType, InSeverity, static_cast<Lumen::UInt32>( InCode ), InLength, InText );
    }
}

[[nodiscard]] Lumen::RHI::ERHIDebugSeverity MapDebugSeverity ( const Lumen::RHIOpenGL::GLenum InSeverity ) noexcept
{
    using Lumen::RHI::ERHIDebugSeverity;
    switch ( InSeverity )
    {
    case Lumen::RHIOpenGL::GlDebugSeverityHigh:
        return ERHIDebugSeverity::Error;
    case Lumen::RHIOpenGL::GlDebugSeverityMedium:
        return ERHIDebugSeverity::Warning;
    default:
        return ERHIDebugSeverity::Info;
    }
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLContext>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLContext::Create ( const Bool bEnableDebug ) noexcept
{
    TSharedRef<FOpenGLContext> Context = MakeShared<FOpenGLContext>( bEnableDebug );
    LUMEN_TRY_EXPECT_ERROR( Context->Start() );
    return Context;
}

Lumen::RHIOpenGL::FOpenGLContext::~FOpenGLContext () noexcept
{
    ( void )WorkerThread.RequestStop();
    Wake.NotifyAll();
    WorkerThread.Join();
    /* The worker drains deferred releases on the GL thread before shutting down
     * the native context; Shutdown() here only closes the queue to later Enqueues. */
    DeferredReleases.Shutdown();
}

const Lumen::RHIOpenGL::FGLDispatchTable &Lumen::RHIOpenGL::FOpenGLContext::GetDispatch () const noexcept
{
    return Dispatch;
}

Lumen::RHI::FRHITimelineValue Lumen::RHIOpenGL::FOpenGLContext::GetCompletedTimeline () const noexcept
{
    return RHI::FRHITimelineValue{ CompletedTimeline.load() };
}

Lumen::RHIOpenGL::FOpenGLWaitState *Lumen::RHIOpenGL::FOpenGLContext::GetWaitState () noexcept
{
    return &WaitState;
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLContext::IsExecutorThread () const noexcept
{
    return WorkerThread.IsJoinable() and FThread::GetCurrentId() == WorkerThread.GetId();
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLContext::ExecuteOnGLThread ( TFunction<void()> InTask ) noexcept
{
    if ( IsExecutorThread() )
    {
        InTask();
        return {};
    }
    return InvokeOnGLThread( static_cast<TFunction<void()> &&>( InTask ) );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLContext::InvokeOnGLThread ( TFunction<void()> InTask ) noexcept
{
    LUMEN_ASSERT( not IsExecutorThread() );
    if ( not WorkerThread.IsJoinable() )
    {
        return TUnexpected( RHI::MakeRHIDeviceLost( RHI::ERHIErrorCondition::Device, "OpenGL executor is stopped." ) );
    }

    TSharedRef<FTaskLatch> Latch = MakeTaskLatch();
    {
        HAL::TLockGuard Lock( QueueMutex );
        Tasks.push( [Task = static_cast<TFunction<void()> &&>( InTask ), Latch] () noexcept
        {
            Task();
            HAL::TLockGuard LatchLock( Latch->Mutex );
            Latch->bFinished = true;
            Latch->Done.NotifyAll();
        } );
    }
    Wake.NotifyOne();

    HAL::TUniqueLock Lock( Latch->Mutex );
    Latch->Done.Wait( Lock, [&Latch] () noexcept -> Bool
    {
        return Latch->bFinished;
    } );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLContext::Submit ( FOpenGLSubmission &&InSubmission ) noexcept
{
    {
        HAL::TLockGuard Lock( SchedulerMutex );
        Pending.push( static_cast<FOpenGLSubmission &&>( InSubmission ) );
    }
    Wake.NotifyOne();
    return {};
}

void Lumen::RHIOpenGL::FOpenGLContext::SetDebugSink ( RHI::IRHIDebugSink *InSink ) noexcept
{
    HAL::TLockGuard Lock( DebugSinkMutex );
    DebugSink = InSink;
}

void Lumen::RHIOpenGL::FOpenGLContext::MarkDeviceLost () noexcept
{
    bDeviceLost.store( true );
    {
        HAL::TLockGuard Lock( WaitState.Mutex );
        WaitState.bUnavailable = true;
    }
    WaitState.Condition.NotifyAll();

    if ( IsExecutorThread() )
    {
        FailAllSubmissions();
    }
    else
    {
        ( void )ExecuteOnGLThread( [this] () noexcept
        {
            FailAllSubmissions();
        } );
    }
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLContext::IsDeviceLost () const noexcept
{
    return bDeviceLost.load();
}

void Lumen::RHIOpenGL::FOpenGLContext::EmitDebugMessage ( RHI::FRHIDebugMessage InMessage ) noexcept
{
    RHI::IRHIDebugSink *Sink = nullptr;
    {
        HAL::TLockGuard Lock( DebugSinkMutex );
        Sink = DebugSink;
    }
    if ( Sink != nullptr )
    {
        Sink->Emit( InMessage );
    }
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLContext::DeferRelease ( const RHI::FRHITimelineValue InRequired,
                                                                          TFunction<void()> InRelease ) noexcept
{
    /* Always retire through the queue: Poll()/Drain() run on the executor, so the
     * native GL calls stay on the GL thread even when the value is already complete. */
    return DeferredReleases.Enqueue( InRequired, static_cast<TFunction<void()> &&>( InRelease ) );
}

/**
 * Private
 */

Lumen::RHIOpenGL::FOpenGLContext::FOpenGLContext ( const Bool bDebugEnabled ) noexcept : bEnableDebug( bDebugEnabled )
{
    /* */
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLContext::Start () noexcept
{
    WorkerThread = FThread( [this] ( FStopToken InStopToken ) noexcept
    {
        Worker( InStopToken );
    } );
    if ( not WorkerThread.IsJoinable() )
    {
        return TUnexpected(
            RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Device, "Unable to start the OpenGL Executor thread." ) );
    }

    {
        HAL::TUniqueLock Lock( QueueMutex );
        Wake.Wait( Lock, [this] () noexcept -> Bool
        {
            return bReady;
        } );
        if ( InitError )
        {
            return TUnexpected( *InitError );
        }
    }
    return {};
}

void Lumen::RHIOpenGL::FOpenGLContext::NotifyDebugMessage ( const GLenum InSource,
                                                            const GLenum InType,
                                                            const GLenum InSeverity,
                                                            const UInt32 InCode,
                                                            const GLsizei InLength,
                                                            const AnsiChar *InText ) const noexcept
{
    FStringView Text = InLength > 0 ? FStringView( InText, static_cast<USize>( InLength ) ) : FStringView( InText );
    RHI::FRHIDebugMessage Message(
        MapDebugSeverity( InSeverity ),
        InType == GlDebugTypeErrorCode ? RHI::ERHIDebugCategory::Validation : RHI::ERHIDebugCategory::General, InCode, Text );
    ( void )InSource;

    RHI::IRHIDebugSink *Sink = nullptr;
    {
        HAL::TLockGuard Lock( DebugSinkMutex );
        const Float64 Now   = HAL::FPlatformTime::Seconds();
        const auto Previous = RecentMessages.find( InCode );
        if ( Previous != RecentMessages.end() and Now - Previous->second < 0.25 )
        {
            return;
        }
        RecentMessages[InCode] = Now;
        if ( RecentMessages.size() > 64U )
        {
            RecentMessages.clear();
        }
        Sink = DebugSink;
    }
    if ( Sink != nullptr )
    {
        Sink->Emit( Message );
    }
}

void Lumen::RHIOpenGL::FOpenGLContext::TrackQueryPoll ( TFunction<Bool()> InPoll ) noexcept
{
    if ( not InPoll )
    {
        return;
    }
    HAL::TLockGuard Lock( SchedulerMutex );
    PendingQueryPolls.push_back( static_cast<TFunction<Bool()> &&>( InPoll ) );
}

void Lumen::RHIOpenGL::FOpenGLContext::PollPendingQueries () noexcept
{
    TVector<TFunction<Bool()>> Polls;
    {
        HAL::TLockGuard Lock( SchedulerMutex );
        Polls = static_cast<TVector<TFunction<Bool()>> &&>( PendingQueryPolls );
        PendingQueryPolls.clear();
    }
    for ( TFunction<Bool()> &Poll : Polls )
    {
        if ( Poll and not Poll() )
        {
            HAL::TLockGuard Lock( SchedulerMutex );
            PendingQueryPolls.push_back( static_cast<TFunction<Bool()> &&>( Poll ) );
        }
    }
}

void Lumen::RHIOpenGL::FOpenGLContext::PollResetStatus () noexcept
{
    if ( Dispatch.GetGraphicsResetStatus == nullptr or bResetReported.load() )
    {
        return;
    }
    const GLenum Status = Dispatch.GetGraphicsResetStatus();
    if ( Status == GlNoError )
    {
        return;
    }
    bResetReported.store( true );

    EmitDebugMessage( RHI::FRHIDebugMessage( RHI::ERHIDebugSeverity::Error, RHI::ERHIDebugCategory::General,
                                             static_cast<UInt32>( Status ), "The OpenGL graphics context was reset." ) );
    MarkDeviceLost();
}

void Lumen::RHIOpenGL::FOpenGLContext::StepScheduler () noexcept
{
    {
        HAL::TLockGuard Lock( SchedulerMutex );
        while ( not Pending.empty() )
        {
            Blocked.push_back( static_cast<FOpenGLSubmission &&>( Pending.front() ) );
            Pending.pop();
        }
    }

    PollPendingQueries();

    for ( USize Index = 0U; Index < Blocked.size(); )
    {
        const Bool bDependenciesReady = not Algo::AnyOf( Blocked[Index].Waits, [] ( const auto &InWait ) noexcept -> Bool
        {
            return not InWait.first or InWait.first->GetValue() < InWait.second;
        } );
        if ( bDependenciesReady )
        {
            FOpenGLSubmission Ready = static_cast<FOpenGLSubmission &&>( Blocked[Index] );
            Blocked.erase( Blocked.begin() + static_cast<ISize>( Index ) );

            const RHI::FRHIExpected Replayed = Ready.Replay ? Ready.Replay() : RHI::FRHIExpected{};
            if ( not Replayed )
            {
                EmitDebugMessage( RHI::FRHIDebugMessage( RHI::ERHIDebugSeverity::Error, RHI::ERHIDebugCategory::General, 0U,
                                                         Replayed.Error().Message ) );
                if ( Ready.OnFailure )
                {
                    Ready.OnFailure( Replayed.Error() );
                }
            }

            GLsync Sync = Dispatch.FenceSync != nullptr ? Dispatch.FenceSync( GlSyncGpuCommandsComplete, 0U ) : nullptr;
            if ( Dispatch.Flush != nullptr )
            {
                Dispatch.Flush();
            }
            if ( Sync == nullptr )
            {
                PollPendingQueries();
                Algo::ForEach( Ready.Signals,
                               [] ( const TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64> &InSignal ) noexcept -> Bool
                {
                    return static_cast<Bool>( InSignal.first );
                }, [] ( const TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64> &InSignal ) noexcept
                {
                    ( void )InSignal.first->Signal( InSignal.second );
                } );
                if ( Ready.OnComplete )
                {
                    Ready.OnComplete();
                }
                const UInt64 NoFenceValue = Ready.Completion.Value;
                if ( NoFenceValue > CompletedTimeline.load() )
                {
                    CompletedTimeline.store( NoFenceValue );
                }
                ( void )DeferredReleases.Poll( RHI::FRHITimelineValue{ CompletedTimeline.load() } );
            }
            else
            {
                FActiveFence Fence;
                Fence.Sync       = Sync;
                Fence.Submission = static_cast<FOpenGLSubmission &&>( Ready );
                ActiveFences.push_back( static_cast<FActiveFence &&>( Fence ) );
            }
            continue;
        }
        ++Index;
    }

    PollPendingQueries();

    for ( USize Index = 0U; Index < ActiveFences.size(); )
    {
        FActiveFence &Fence = ActiveFences[Index];
        const GLenum Status = Dispatch.ClientWaitSync( Fence.Sync, GlSyncFlushCommandsBit, 0ULL );
        if ( Status == GlSyncTimeoutExpired )
        {
            ++Index;
            continue;
        }
        if ( Dispatch.DeleteSync != nullptr )
        {
            Dispatch.DeleteSync( Fence.Sync );
        }
        FOpenGLSubmission Completed = static_cast<FOpenGLSubmission &&>( Fence.Submission );
        ActiveFences.erase( ActiveFences.begin() + static_cast<ISize>( Index ) );

        /** M4.2: publish query results before the queue completion value becomes visible. */
        PollPendingQueries();
        Algo::ForEach( Completed.Signals,
                       [] ( const TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64> &InSignal ) noexcept -> Bool
        {
            return static_cast<Bool>( InSignal.first );
        }, [] ( const TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64> &InSignal ) noexcept
        {
            ( void )InSignal.first->Signal( InSignal.second );
        } );
        if ( Completed.OnComplete )
        {
            Completed.OnComplete();
        }
        const UInt64 Value = Completed.Completion.Value;
        if ( Value > CompletedTimeline.load() )
        {
            CompletedTimeline.store( Value );
        }
        ( void )DeferredReleases.Poll( RHI::FRHITimelineValue{ CompletedTimeline.load() } );
    }
}

void Lumen::RHIOpenGL::FOpenGLContext::FailAllSubmissions () noexcept
{
    LUMEN_ASSERT( IsExecutorThread() );

    TVector<FOpenGLSubmission> Abandoned;
    TVector<GLsync> ObsoleteSyncs;
    {
        HAL::TLockGuard Lock( SchedulerMutex );
        while ( not Pending.empty() )
        {
            Abandoned.push_back( static_cast<FOpenGLSubmission &&>( Pending.front() ) );
            Pending.pop();
        }
        Algo::ForEach( Blocked, [&Abandoned] ( FOpenGLSubmission &InSubmission ) noexcept
        {
            Abandoned.push_back( static_cast<FOpenGLSubmission &&>( InSubmission ) );
        } );
        Blocked.clear();
        Algo::ForEach( ActiveFences, [&Abandoned, &ObsoleteSyncs] ( FActiveFence &InFence ) noexcept
        {
            if ( InFence.Sync != nullptr )
            {
                ObsoleteSyncs.push_back( InFence.Sync );
                InFence.Sync = nullptr;
            }
            Abandoned.push_back( static_cast<FOpenGLSubmission &&>( InFence.Submission ) );
        } );
        ActiveFences.clear();
    }

    /* Native GL calls run outside SchedulerMutex: the debug sink is a rank-6 leaf. */
    if ( Dispatch.DeleteSync != nullptr )
    {
        Algo::ForEach( ObsoleteSyncs, [this] ( GLsync InSync ) noexcept
        {
            Dispatch.DeleteSync( InSync );
        } );
    }

    const RHI::FRHIError Failure = RHI::MakeRHIDeviceLost(
        RHI::ERHIErrorCondition::Queue, "OpenGL submission was abandoned after device loss or executor stop." );
    Algo::ForEach( Abandoned,
                   [] ( FOpenGLSubmission &InSubmission ) noexcept -> Bool
    {
        return static_cast<Bool>( InSubmission.OnFailure );
    }, [&Failure] ( FOpenGLSubmission &InSubmission ) noexcept
    {
        InSubmission.OnFailure( Failure );
    } );
}

void Lumen::RHIOpenGL::FOpenGLContext::Worker ( const FStopToken InStopToken ) noexcept
{
    const TExpected<void, RHI::FRHIError> Initialized = NativeContext.Initialize( bEnableDebug );
    if ( not Initialized )
    {
        InitError = TOptional<RHI::FRHIError>( Initialized.Error() );
    }
    if ( not InitError )
    {
        const TExpected<void, RHI::FRHIError> Loaded = LoadGLDispatchTable( Dispatch, NativeContext );
        if ( not Loaded )
        {
            InitError = TOptional<RHI::FRHIError>( Loaded.Error() );
        }
        else
        {
            Dispatch.ClipControl( GlLowerLeft, GlZeroToOne );
            if ( Dispatch.DebugMessageCallback != nullptr )
            {
                Dispatch.DebugMessageCallback( &DebugMessageThunk, this );
            }
        }
    }

    {
        HAL::TLockGuard Lock( QueueMutex );
        bReady = true;
    }
    Wake.NotifyAll();
    if ( InitError )
    {
        NativeContext.Shutdown();
        return;
    }

    for ( ;; )
    {
        TFunction<void()> Task;
        {
            HAL::TUniqueLock Lock( QueueMutex );
            ( void )Wake.WaitFor( Lock, 0.01, [this, &InStopToken] () noexcept -> Bool
            {
                return not Tasks.empty() or InStopToken.StopRequested();
            } );
            if ( not Tasks.empty() )
            {
                Task = static_cast<TFunction<void()> &&>( Tasks.front() );
                Tasks.pop();
            }
        }
        if ( Task )
        {
            Task();
        }

        StepScheduler();
        PollResetStatus();

        /* A submission blocked on a semaphore that is never signalled would keep the
         * worker alive forever. Fail and release all not-yet-replayed work on stop. */
        if ( InStopToken.StopRequested() )
        {
            FailAllSubmissions();
        }

        Bool bPending = false;
        {
            HAL::TLockGuard Lock( SchedulerMutex );
            bPending = not Pending.empty();
        }
        if ( InStopToken.StopRequested() and not Task and not bPending and Blocked.empty() and ActiveFences.empty() )
        {
            break;
        }
    }

    if ( Dispatch.DebugMessageCallback != nullptr )
    {
        Dispatch.DebugMessageCallback( nullptr, nullptr );
    }
    ( void )DeferredReleases.Drain();
    NativeContext.Shutdown();
}
