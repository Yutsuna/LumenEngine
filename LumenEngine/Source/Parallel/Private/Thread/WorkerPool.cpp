/**
 * @file WorkerPool.cpp
 * @brief Implementation of the high-performance worker pool.
 */

#include "Thread/WorkerPool.hpp"

#include "Container/UniquePtr.hpp"

#include <bit>
#include <cassert>

namespace
{

thread_local LumenEngine::Bool bLumenIsWorkerThread      = false;
thread_local LumenEngine::UInt32 LumenCurrentWorkerIndex = 0U;

/** INFO: Each worker thread gets a 1MB scratch buffer for transient allocations */
constexpr LumenEngine::USize LumenWorkerScratchSize = 1024ULL * 1024ULL;
thread_local LumenEngine::TUniquePtr<LumenEngine::Byte[]> LumenWorkerScratchBuffer;
thread_local LumenEngine::TOptional<LumenEngine::HAL::FLinearAllocator> LumenWorkerAllocator;

} // namespace

LumenEngine::Parallel::FWorkerPool::FWorkerPool ( UInt32 InWorkerCount, USize InQueueCapacity ) noexcept
    : Queue( std::bit_ceil( InQueueCapacity < 2ULL ? 2ULL : InQueueCapacity ) ),
      //
      TaskSemaphore( 0L ),
      //
      ActiveTaskCount( 0U ),
      //
      bIsActive( true ),
      //
      WorkerExecutionCounts( InWorkerCount )
{
    Workers.reserve( InWorkerCount );

    for ( UInt32 Index = 0U; Index < InWorkerCount; ++Index )
    {
        Workers.emplace_back( [this, Index] () { WorkerLoop( Index ); } );
    }
}

LumenEngine::Parallel::FWorkerPool::~FWorkerPool () noexcept
{
    Shutdown();
}

LumenEngine::Bool LumenEngine::Parallel::FWorkerPool::Submit ( FTask InTask ) noexcept
{
    if ( not bIsActive.load( std::memory_order_acquire ) )
    {
        return false;
    }

    ActiveTaskCount.fetch_add( 1U, std::memory_order_acquire );

    if ( Queue.Push( std::move( InTask ) ) )
    {
        TaskSemaphore.release();
        return true;
    }

    /** INFO: Push failed because the bounded queue is at maximum capacity */
    ActiveTaskCount.fetch_sub( 1U, std::memory_order_release );
    ActiveTaskCount.notify_all();

    return false;
}

void LumenEngine::Parallel::FWorkerPool::WaitAll () noexcept
{
    UInt32 Count = ActiveTaskCount.load( std::memory_order_acquire );

    while ( Count > 0U )
    {
        ActiveTaskCount.wait( Count, std::memory_order_acquire );
        Count = ActiveTaskCount.load( std::memory_order_acquire );
    }
}

void LumenEngine::Parallel::FWorkerPool::Shutdown () noexcept
{
    const Bool bWasActive = bIsActive.exchange( false, std::memory_order_release );

    if ( not bWasActive )
    {
        return;
    }

    /** INFO: Unblock all sleeping threads */
    for ( USize Index = 0ULL; Index < Workers.size(); ++Index )
    {
        TaskSemaphore.release();
    }

    /** INFO: Graceful wait for currently executing tasks to resolve */
    for ( std::jthread &Worker : Workers )
    {
        if ( Worker.joinable() )
        {
            Worker.join();
        }
    }

    Workers.clear();
}

LumenEngine::TOptional<LumenEngine::UInt32> LumenEngine::Parallel::FWorkerPool::GetCurrentWorkerIndex () noexcept
{
    if ( not bLumenIsWorkerThread )
    {
        return {};
    }

    return LumenCurrentWorkerIndex;
}

LumenEngine::HAL::FLinearAllocator &LumenEngine::Parallel::FWorkerPool::GetWorkerAllocator () noexcept
{
    assert( LumenWorkerAllocator.has_value() and "GetWorkerAllocator() called from a non-worker thread or before initialization." );
    return *LumenWorkerAllocator;
}

LumenEngine::TVector<LumenEngine::UInt64> LumenEngine::Parallel::FWorkerPool::GetWorkerExecutionCounts () const noexcept
{
    TVector<UInt64> Result;
    Result.reserve( WorkerExecutionCounts.size() );

    for ( const TAtomic<UInt64> &Counter : WorkerExecutionCounts )
    {
        Result.push_back( Counter.load( std::memory_order_relaxed ) );
    }

    return Result;
}

LumenEngine::UInt64 LumenEngine::Parallel::FWorkerPool::GetTotalExecutedTaskCount () const noexcept
{
    UInt64 Total = 0ULL;

    for ( const TAtomic<UInt64> &Counter : WorkerExecutionCounts )
    {
        Total += Counter.load( std::memory_order_relaxed );
    }

    return Total;
}

void LumenEngine::Parallel::FWorkerPool::WorkerLoop ( UInt32 InWorkerIndex ) noexcept
{
    bLumenIsWorkerThread    = true;
    LumenCurrentWorkerIndex = InWorkerIndex;

    LumenWorkerScratchBuffer = MakeUnique<Byte[]>( LumenWorkerScratchSize );
    LumenWorkerAllocator.emplace( LumenWorkerScratchBuffer.Get(), LumenWorkerScratchSize );

    while ( bIsActive.load( std::memory_order_acquire ) )
    {
        TaskSemaphore.acquire();

        if ( not bIsActive.load( std::memory_order_acquire ) )
        {
            break;
        }

        if ( TOptional<FTask> Task = Queue.Pop() )
        {
            {
                HAL::FScopeLinear Scope( *LumenWorkerAllocator );
                ( *Task )();
            }

            WorkerExecutionCounts[InWorkerIndex].fetch_add( 1ULL, std::memory_order_relaxed );

            ActiveTaskCount.fetch_sub( 1U, std::memory_order_release );
            ActiveTaskCount.notify_all();
        }
    }

    LumenWorkerAllocator.reset();
    LumenWorkerScratchBuffer.Reset();

    bLumenIsWorkerThread = false;
}