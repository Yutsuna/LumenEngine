/**
 * @file OpenGLContext.cpp
 * @brief Implements the single-threaded OpenGL executor.
 */

#include <RHIOpenGL/Device/OpenGLContext.hpp>

#include <HAL/LockGuard.hpp>

#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Loader/GLLoader.hpp>

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

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLContext>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLContext::Create ( const Bool bEnableDebug ) noexcept
{
    TSharedRef<FOpenGLContext> Context = MakeShared<FOpenGLContext>( bEnableDebug );
    if ( const RHI::FRHIExpected Started = Context->Start(); not Started )
    {
        return TUnexpected( Started.Error() );
    }
    return Context;
}

Lumen::RHIOpenGL::FOpenGLContext::~FOpenGLContext () noexcept
{
    ( void )WorkerThread.RequestStop();
    Wake.NotifyAll();
    WorkerThread.Join();
}

const Lumen::RHIOpenGL::FGLDispatchTable &Lumen::RHIOpenGL::FOpenGLContext::GetDispatch () const noexcept
{
    return Dispatch;
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

/**
 * Private
 */

Lumen::RHIOpenGL::FOpenGLContext::FOpenGLContext ( const Bool InEnableDebug ) noexcept : bEnableDebug( InEnableDebug )
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
            Wake.Wait( Lock, [this, &InStopToken] () noexcept -> Bool
            {
                return not Tasks.empty() or InStopToken.StopRequested();
            } );
            if ( Tasks.empty() )
            {
                if ( InStopToken.StopRequested() )
                {
                    break;
                }
                continue;
            }
            Task = static_cast<TFunction<void()> &&>( Tasks.front() );
            Tasks.pop();
        }
        Task();
    }
    NativeContext.Shutdown();
}
