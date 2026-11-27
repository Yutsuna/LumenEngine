/**
 * @file RenderExecutor.cpp
 * @brief Single-thread render command executor implementation.
 */

#include <Execution/RenderExecutor.hpp>

#include <HAL/LockGuard.hpp>
#include <Thread/MPSCQueue.hpp>

namespace
{

/** @brief Translates the bounded queue's capacity failure into the public executor error set. */
[[nodiscard]] Lumen::ERenderExecutorError ToExecutorError ( const Lumen::Thread::EMPSCQueueError InError ) noexcept
{
    switch ( InError )
    {
    case Lumen::Thread::EMPSCQueueError::QueueFull:
        return Lumen::ERenderExecutorError::QueueFull;
    case Lumen::Thread::EMPSCQueueError::NotInitialized:
        return Lumen::ERenderExecutorError::NotRunning;
    default:
        return Lumen::ERenderExecutorError::QueueFull;
    }
}

} // namespace

Lumen::FRenderExecutor::FRenderExecutor () noexcept = default;

Lumen::FRenderExecutor::~FRenderExecutor () noexcept
{
    RequestStop();
    Join();
}

Lumen::TExpected<void, Lumen::ERenderExecutorError> Lumen::FRenderExecutor::Start ( const FRenderExecutorDesc &InDesc,
                                                                                    IRenderCommandHandler &InHandler ) noexcept
{
    if ( bRunning.load( std::memory_order_acquire ) )
    {
        return MakeUnexpected( ERenderExecutorError::AlreadyRunning );
    }
    if ( InDesc.CommandCapacity == 0ULL )
    {
        return MakeUnexpected( ERenderExecutorError::InvalidCapacity );
    }

    TUniquePtr<Thread::TMPSCQueue<FRenderCommand>> Queue       = MakeUnique<Thread::TMPSCQueue<FRenderCommand>>();
    const TExpected<void, Thread::EMPSCQueueError> Initialized = Queue->Initialize( InDesc.CommandCapacity );
    if ( not Initialized ) [[unlikely]]
    {
        return MakeUnexpected( ERenderExecutorError::QueueCreationFailed );
    }

    {
        /** INFO: State transitions are serial control operations; producers are counted before they can enter Enqueue. */
        const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ WakeMutex };
        if ( bRunning.load( std::memory_order_relaxed ) )
        {
            return MakeUnexpected( ERenderExecutorError::AlreadyRunning );
        }
        Commands = std::move( Queue );
        Handler  = &InHandler;

        bStopRequested.store( false, std::memory_order_relaxed );
        ActiveProducers.store( 0ULL, std::memory_order_relaxed );
        bWorkAvailable = false;
        bRunning.store( true, std::memory_order_release );
    }
    {
        const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ StatusMutex };
        WorkingStatus = FRenderFrameStatus{};
    }
    PublishStatus();

    WorkerThread = FThread( [this] ( FStopToken InStopToken ) noexcept
    {
        Run( std::move( InStopToken ) );
    } );
    if ( not WorkerThread.IsJoinable() ) [[unlikely]]
    {
        {
            const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ WakeMutex };
            bRunning.store( false, std::memory_order_release );
            Handler = nullptr;
            Commands.Reset();
        }
        PublishStatus();
        return MakeUnexpected( ERenderExecutorError::ThreadCreationFailed );
    }

    return {};
}

Lumen::TExpected<void, Lumen::ERenderExecutorError> Lumen::FRenderExecutor::Enqueue ( const FRenderCommand &InCommand ) noexcept
{
    /**
     * INFO: A producer is counted before it reads the acceptance flags; Join waits for every counted producer to leave
     *       before releasing the queue. This keeps the hot path lock-free on rejection without a reset/UAF window.
     */
    ActiveProducers.fetch_add( 1ULL, std::memory_order_acq_rel );
    if ( not bRunning.load( std::memory_order_acquire ) or bStopRequested.load( std::memory_order_acquire ) )
    {
        ActiveProducers.fetch_sub( 1ULL, std::memory_order_release );
        RejectedCommandCount.fetch_add( 1ULL, std::memory_order_relaxed );
        PublishStatus();
        return MakeUnexpected( ERenderExecutorError::NotRunning );
    }

    const TExpected<void, Thread::EMPSCQueueError> Result = Commands->Push( InCommand );
    if ( not Result ) [[unlikely]]
    {
        /** INFO: QueueFull is normal backpressure; the worker publishes the counter on its next command. */
        ActiveProducers.fetch_sub( 1ULL, std::memory_order_release );
        RejectedCommandCount.fetch_add( 1ULL, std::memory_order_relaxed );
        return MakeUnexpected( ToExecutorError( Result.Error() ) );
    }

    SignalWork();
    ActiveProducers.fetch_sub( 1ULL, std::memory_order_release );
    return {};
}

void Lumen::FRenderExecutor::RequestStop () noexcept
{
    const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ WakeMutex };
    bStopRequested.store( true, std::memory_order_release );
    bRunning.store( false, std::memory_order_release );
    WakeCondition.NotifyAll();
}

void Lumen::FRenderExecutor::Join () noexcept
{
    RequestStop();
    if ( WorkerThread.IsJoinable() )
    {
        WorkerThread.Join();
    }
    /** INFO: Wait for producers that had already passed the acceptance check before stop was published. */
    while ( ActiveProducers.load( std::memory_order_acquire ) != 0ULL )
    {
        FThread::Yield();
    }
    {
        const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ WakeMutex };
        Commands.Reset();
        Handler = nullptr;
    }
    /** INFO: The worker has exited; this is the final status publication, including post-stop rejections. */
    PublishStatus();
}

Lumen::Bool Lumen::FRenderExecutor::IsRunning () const noexcept
{
    return bRunning.load( std::memory_order_acquire );
}

void Lumen::FRenderExecutor::SwapStatusBuffers () noexcept
{
    StatusBuffer.SwapReadBuffers();
}

const Lumen::FRenderFrameStatus &Lumen::FRenderExecutor::GetStatus () const noexcept
{
    return StatusBuffer.ReadBuffer();
}

void Lumen::FRenderExecutor::Run ( FStopToken InStopToken ) noexcept
{
    /** INFO: The handler mutates a worker-local copy; only publication touches the shared WorkingStatus. */
    Bool bFinalDrainDone = false;
    FRenderFrameStatus LocalStatus;
    {
        const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ StatusMutex };
        LocalStatus = WorkingStatus;
    }

    for ( ;; )
    {
        {
            const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ WakeMutex };
            bWorkAvailable = false;
        }

        for ( ;; )
        {
            const TOptional<FRenderCommand> Command = Commands->Pop();
            if ( not Command )
            {
                break;
            }
            Handler->HandleRenderCommand( *Command, LocalStatus );
            {
                const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ StatusMutex };
                WorkingStatus = LocalStatus;
                PublishStatusLocked();
            }
        }

        if ( bStopRequested.load( std::memory_order_acquire ) or InStopToken.StopRequested() )
        {
            /**
             * INFO: A producer that passed acceptance before stop may still be publishing its push. Wait for it, then
             *       perform exactly one final drain so no accepted command is destroyed without being handled.
             */
            if ( bFinalDrainDone )
            {
                break;
            }
            while ( ActiveProducers.load( std::memory_order_acquire ) != 0ULL )
            {
                FThread::Yield();
            }
            bFinalDrainDone = true;
            continue;
        }

        WakeMutex.Lock();
        WakeCondition.Wait( WakeMutex, [this, &InStopToken] () noexcept -> Bool
        {
            return bWorkAvailable or bStopRequested.load( std::memory_order_acquire ) or InStopToken.StopRequested();
        } );
        WakeMutex.Unlock();
    }

    {
        const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ StatusMutex };
        WorkingStatus = LocalStatus;
        PublishStatusLocked();
    }
}

void Lumen::FRenderExecutor::SignalWork () noexcept
{
    const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ WakeMutex };
    bWorkAvailable = true;
    WakeCondition.NotifyOne();
}

void Lumen::FRenderExecutor::PublishStatus () noexcept
{
    const HAL::TLockGuard<HAL::FPlatformMutex> LockGuard{ StatusMutex };
    PublishStatusLocked();
}

void Lumen::FRenderExecutor::PublishStatusLocked () noexcept
{
    WorkingStatus.RejectedCommandCount = RejectedCommandCount.load( std::memory_order_relaxed );
    StatusBuffer.WriteBuffer( WorkingStatus );
}
