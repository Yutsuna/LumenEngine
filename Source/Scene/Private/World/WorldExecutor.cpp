/**
 * @file WorldExecutor.cpp
 * @brief Private bounded world task executor implementation.
 */

#include <World/WorldExecutor.hpp>

#include <Actor/Actor.hpp>
#include <HAL/LockGuard.hpp>
#include <HAL/PlatformFutex.hpp>

#include <limits>

Lumen::FWorldExecutor::FWorldExecutor ( const UInt32 InWorkerCount, const USize InTaskCapacity ) noexcept
    : ExpectedWorkerCount( InWorkerCount )
{
    if ( InTaskCapacity == 0ULL or InWorkerCount == 0U )
    {
        return;
    }

    TaskRing.resize( InTaskCapacity );
    Workers.reserve( InWorkerCount );
    for ( UInt32 WorkerIndex = 0U; WorkerIndex < InWorkerCount; ++WorkerIndex )
    {
        Workers.emplace_back( [this] ( FStopToken InStopToken ) noexcept
        {
            WorkerMain( InStopToken );
        } );
    }
}

Lumen::FWorldExecutor::~FWorldExecutor () noexcept
{
    {
        HAL::TUniqueLock<HAL::FPlatformMutex> Lock( QueueMutex );
        bStopping = true;
    }

    for ( FThread &Worker : Workers )
    {
        static_cast<void>( Worker.RequestStop() );
    }
    WorkAvailable.NotifyAll();

    for ( FThread &Worker : Workers )
    {
        Worker.Join();
    }
}

Lumen::Bool Lumen::FWorldExecutor::IsValid () const noexcept
{
    if ( TaskRing.empty() or Workers.size() != ExpectedWorkerCount )
    {
        return false;
    }

    for ( const FThread &Worker : Workers )
    {
        if ( not Worker.IsJoinable() )
        {
            return false;
        }
    }
    return true;
}

Lumen::TExpected<Lumen::FWorldPhaseCompletion, Lumen::EWorldTickError>
Lumen::FWorldExecutor::ExecutePhase ( const TSpan<AActor *const> InActors,
                                      const FWorldFrameContext &InContext,
                                      const Float64 InDeltaTime,
                                      const UInt32 InMaxMessages ) noexcept
{
    FWorldPhaseCompletion Completion{ .Context = InContext, .CompletedTaskCount = 0ULL, .bCapacityError = false };
    Completion.Context.TaskCount = InActors.size();

    if ( not IsValid() )
    {
        return MakeUnexpected( EWorldTickError::ExecutorUnavailable );
    }
    if ( InActors.empty() )
    {
        return Completion;
    }
    if ( InActors.size() > TaskRing.size() or InActors.size() > static_cast<USize>( std::numeric_limits<UInt32>::max() ) )
    {
        Completion.bCapacityError = true;
        return MakeUnexpected( EWorldTickError::PhaseCapacityExceeded );
    }

    const USize TaskCount = InActors.size();
    RemainingTasks.store( static_cast<UInt32>( TaskCount ), std::memory_order_release );

    {
        HAL::TUniqueLock<HAL::FPlatformMutex> Lock( QueueMutex );
        if ( bStopping or QueuedTaskCount != 0ULL )
        {
            RemainingTasks.store( 0U, std::memory_order_release );
            return MakeUnexpected( EWorldTickError::ExecutorUnavailable );
        }

        for ( USize TaskIndex = 0ULL; TaskIndex < TaskCount; ++TaskIndex )
        {
            const USize RingIndex = ( QueueTail + TaskIndex ) % TaskRing.size();
            TaskRing[RingIndex]   = FTask{ .Actor       = InActors[TaskIndex],
                                           .Context     = Completion.Context,
                                           .DeltaTime   = InDeltaTime,
                                           .MaxMessages = InMaxMessages };
        }
        QueueTail       = ( QueueTail + TaskCount ) % TaskRing.size();
        QueuedTaskCount = TaskCount;
    }
    WorkAvailable.NotifyAll();

    UInt32 ObservedRemaining = RemainingTasks.load( std::memory_order_acquire );
    while ( ObservedRemaining != 0U )
    {
        HAL::FPlatformFutex::Wait( RemainingTasks, ObservedRemaining );
        ObservedRemaining = RemainingTasks.load( std::memory_order_acquire );
    }

    Completion.CompletedTaskCount = TaskCount;
    return Completion;
}

void Lumen::FWorldExecutor::WorkerMain ( const FStopToken InStopToken ) noexcept
{
    while ( not InStopToken.StopRequested() )
    {
        FTask Task;
        {
            HAL::TUniqueLock<HAL::FPlatformMutex> Lock( QueueMutex );
            WorkAvailable.Wait( Lock, [this, &InStopToken] () noexcept -> Bool
            {
                return bStopping or InStopToken.StopRequested() or QueuedTaskCount != 0ULL;
            } );

            if ( bStopping or InStopToken.StopRequested() )
            {
                return;
            }

            Task      = TaskRing[QueueHead];
            QueueHead = ( QueueHead + 1ULL ) % TaskRing.size();
            --QueuedTaskCount;
        }

        Task.Actor->RunWorldFrame( Task.Context, Task.DeltaTime, Task.MaxMessages );
        if ( RemainingTasks.fetch_sub( 1U, std::memory_order_acq_rel ) == 1U )
        {
            HAL::FPlatformFutex::WakeAll( RemainingTasks );
        }
    }
}
