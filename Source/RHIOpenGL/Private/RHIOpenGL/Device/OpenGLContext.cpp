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

#include <RHIOpenGL/Device/OpenGLFenceStatus.hpp>
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

/** @brief Test seam state; see FOpenGLContext::SetForcedInitFailureForTests. */
Lumen::TAtomic<Lumen::Bool> GForceInitFailure{ false };

/** @brief Terminal error for work abandoned by a graphics reset. */
[[nodiscard]] Lumen::RHI::FRHIError MakeLossAbandonError () noexcept
{
    return Lumen::RHI::MakeRHIDeviceLost( Lumen::RHI::ERHIErrorCondition::Queue,
                                          "OpenGL submission was abandoned after device loss." );
}

/** @brief Terminal error for work abandoned by executor shutdown. */
[[nodiscard]] Lumen::RHI::FRHIError MakeStopAbandonError () noexcept
{
    return Lumen::RHI::MakeRHIDeviceLost( Lumen::RHI::ERHIErrorCondition::Queue,
                                          "OpenGL submission was abandoned after executor stop." );
}

/** @brief Terminal error for work abandoned by a clean device closure. */
[[nodiscard]] Lumen::RHI::FRHIError MakeCloseAbandonError () noexcept
{
    return { Lumen::RHI::ERHIErrorCode::DeviceClosed, Lumen::RHI::ERHIErrorCondition::Queue,
             "OpenGL submission was abandoned after device closure." };
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

void Lumen::RHIOpenGL::FOpenGLContext::SetForcedInitFailureForTests ( const Bool bForce ) noexcept
{
    GForceInitFailure.store( bForce );
}

Lumen::RHIOpenGL::FOpenGLContext::~FOpenGLContext () noexcept
{
    Shutdown();
}

void Lumen::RHIOpenGL::FOpenGLContext::Shutdown () noexcept
{
    /* One atomic transition claims the stop: a concurrent caller returns at once, and
     * InvokeOnGLThread starts rejecting new work before any task is posted. */
    if ( bStopping.exchange( true ) )
    {
        return;
    }

    if ( WorkerThread.IsJoinable() and IsExecutorThread() )
    {
        /* A self-destruction cannot post and wait for a task on its own worker. */
        DeferredReleases.Shutdown();
    }
    else if ( WorkerThread.IsJoinable() )
    {
        /* The gate is already closed; the internal path still lets the worker retire GL handles. */
        ( void )PostTaskAndWait( [this] () noexcept
        {
            DeferredReleases.Shutdown();
        }, true );
    }

    ( void )WorkerThread.RequestStop();
    Wake.NotifyAll();

    /* A self-destruction cannot join its own worker; the loop exits after the task. */
    if ( WorkerThread.IsJoinable() and not IsExecutorThread() )
    {
        WorkerThread.Join();
    }
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
    return ExecutorThreadId.load() == FThread::GetCurrentId();
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
    return PostTaskAndWait( static_cast<TFunction<void()> &&>( InTask ), false );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLContext::PostTaskAndWait ( TFunction<void()> InTask,
                                                                             const Bool bAllowWhileStopping ) noexcept
{
    TSharedRef<FTaskLatch> Latch = MakeTaskLatch();
    {
        HAL::TLockGuard Lock( QueueMutex );
        /* The gate check and the enqueue share QueueMutex, so no accepted task can be
         * stranded by the worker's exit check. The gate itself is claimed atomically before
         * the drain is posted, or by Start() when initialization fails. */
        if ( not bAllowWhileStopping and bStopping.load() )
        {
            return TUnexpected(
                RHI::MakeRHIDeviceLost( RHI::ERHIErrorCondition::Device, "OpenGL executor is stopping or stopped." ) );
        }
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
    /* Loss wakes waiters immediately: the reset path must not wait for a stuck replay. */
    PublishUnavailable();

    if ( IsExecutorThread() )
    {
        FailAllSubmissions( MakeLossAbandonError() );
    }
    else
    {
        ( void )ExecuteOnGLThread( [this] () noexcept
        {
            FailAllSubmissions( MakeLossAbandonError() );
        } );
    }
}

void Lumen::RHIOpenGL::FOpenGLContext::MarkDeviceClosed () noexcept
{
    const auto FailAndWake = [this] () noexcept
    {
        FailAllSubmissions( MakeCloseAbandonError() );
        /* D7 ordering: queue completions are published by OnFailure before waiters wake. */
        PublishUnavailable();
    };

    if ( IsExecutorThread() )
    {
        FailAndWake();
        return;
    }
    if ( not ExecuteOnGLThread( FailAndWake ) )
    {
        /* The executor is already stopping/stopped; Worker() has retired the work. */
        PublishUnavailable();
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
    if ( RHI::IsRHICompletionReached( RHI::FRHITimelineValue{ CompletedTimeline.load() }, InRequired ) )
    {
        return ExecuteOnGLThread( static_cast<TFunction<void()> &&>( InRelease ) );
    }
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

    TOptional<RHI::FRHIError> StartupError;
    {
        HAL::TUniqueLock Lock( QueueMutex );
        Wake.Wait( Lock, [this] () noexcept -> Bool
        {
            return bReady;
        } );
        if ( InitError )
        {
            StartupError = InitError;
            /* The worker has published readiness and will not enter its task loop. Claim
             * the stop here: Shutdown() must not post a drain nobody would execute. */
            bStopping.store( true );
        }
    }
    if ( StartupError )
    {
        ( void )WorkerThread.RequestStop();
        Wake.NotifyAll();
        if ( WorkerThread.IsJoinable() and not IsExecutorThread() )
        {
            WorkerThread.Join();
        }
        return TUnexpected( *StartupError );
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

void Lumen::RHIOpenGL::FOpenGLContext::RetireSubmission ( FOpenGLSubmission &&InSubmission,
                                                          const RHI::FRHIExpected &InResult ) noexcept
{
    LUMEN_ASSERT( IsExecutorThread() );

    /** M4.2: publish query results before the queue completion value becomes visible. */
    PollPendingQueries();

    /* Advance before the callback: a waiter released by OnComplete/OnFailure may destroy
     * retained objects, and DeferRelease then sees the value already reached. */
    const UInt64 Value = InSubmission.Completion.Value;
    if ( Value > CompletedTimeline.load() )
    {
        CompletedTimeline.store( Value );
    }
    ( void )DeferredReleases.Poll( RHI::FRHITimelineValue{ CompletedTimeline.load() } );

    if ( InResult )
    {
        Algo::ForEach( InSubmission.Signals,
                       [] ( const TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64> &InSignal ) noexcept -> Bool
        {
            return static_cast<Bool>( InSignal.first );
        }, [] ( const TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64> &InSignal ) noexcept
        {
            ( void )InSignal.first->Signal( InSignal.second );
        } );
        if ( InSubmission.OnComplete )
        {
            InSubmission.OnComplete();
        }
    }
    else
    {
        EmitDebugMessage( RHI::FRHIDebugMessage( RHI::ERHIDebugSeverity::Error, RHI::ERHIDebugCategory::General, 0U,
                                                 InResult.Error().Message ) );
        if ( InSubmission.OnFailure )
        {
            InSubmission.OnFailure( InResult.Error() );
        }
    }
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
                /* D7: a failed replay is terminal. No fence, no signals, no OnComplete. */
                RetireSubmission( static_cast<FOpenGLSubmission &&>( Ready ), Replayed );
                continue;
            }

            GLsync Sync = Dispatch.FenceSync != nullptr ? Dispatch.FenceSync( GlSyncGpuCommandsComplete, 0U ) : nullptr;
            if ( Dispatch.Flush != nullptr )
            {
                Dispatch.Flush();
            }
            if ( Sync == nullptr )
            {
                /* FenceSync is a required entry point; this is an unreachable fallback. */
                RetireSubmission( static_cast<FOpenGLSubmission &&>( Ready ), RHI::FRHIExpected{} );
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
        FActiveFence &Fence               = ActiveFences[Index];
        const GLenum Status               = Dispatch.ClientWaitSync( Fence.Sync, GlSyncFlushCommandsBit, 0ULL );
        const EFenceWaitStatus WaitStatus = ClassifyFenceWaitStatus( Status );
        if ( WaitStatus == EFenceWaitStatus::Timeout )
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

        if ( WaitStatus == EFenceWaitStatus::Failed )
        {
            /* glClientWaitSync failed (or returned an unknown result): drain the sticky
             * native error so it cannot poison the next replay's GetError() check. */
            const GLenum NativeError = Dispatch.GetError != nullptr ? Dispatch.GetError() : GlNoError;
            EmitDebugMessage( RHI::FRHIDebugMessage( RHI::ERHIDebugSeverity::Error, RHI::ERHIDebugCategory::General,
                                                     static_cast<UInt32>( NativeError ),
                                                     "glClientWaitSync did not report fence completion." ) );
            RetireSubmission(
                static_cast<FOpenGLSubmission &&>( Completed ),
                TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Queue, "OpenGL fence wait failed." ) ) );
            continue;
        }
        RetireSubmission( static_cast<FOpenGLSubmission &&>( Completed ), RHI::FRHIExpected{} );
    }
}

void Lumen::RHIOpenGL::FOpenGLContext::PublishUnavailable () noexcept
{
    {
        HAL::TLockGuard Lock( WaitState.Mutex );
        WaitState.bUnavailable = true;
    }
    WaitState.Condition.NotifyAll();
}

void Lumen::RHIOpenGL::FOpenGLContext::FailAllSubmissions ( const RHI::FRHIError &InError ) noexcept
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

    Algo::ForEach( Abandoned,
                   [] ( FOpenGLSubmission &InSubmission ) noexcept -> Bool
    {
        return static_cast<Bool>( InSubmission.OnFailure );
    }, [&InError] ( FOpenGLSubmission &InSubmission ) noexcept
    {
        InSubmission.OnFailure( InError );
    } );
}

void Lumen::RHIOpenGL::FOpenGLContext::Worker ( const FStopToken InStopToken ) noexcept
{
    ExecutorThreadId.store( FThread::GetCurrentId() );
    if ( GForceInitFailure.load() )
    {
        InitError = TOptional<RHI::FRHIError>(
            RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Device, "OpenGL initialization failed in tests." ) );
    }
    else
    {
        const TExpected<void, RHI::FRHIError> Initialized = NativeContext.Initialize( bEnableDebug );
        if ( not Initialized )
        {
            InitError = TOptional<RHI::FRHIError>( Initialized.Error() );
        }
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
            FailAllSubmissions( MakeStopAbandonError() );
        }

        Bool bPending = false;
        {
            HAL::TLockGuard Lock( SchedulerMutex );
            bPending = not Pending.empty();
        }
        Bool bTasksQueued = false;
        {
            HAL::TLockGuard Lock( QueueMutex );
            bTasksQueued = not Tasks.empty();
        }
        if ( InStopToken.StopRequested() and not Task and not bTasksQueued and not bPending and Blocked.empty() and
             ActiveFences.empty() )
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
