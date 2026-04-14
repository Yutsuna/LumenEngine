/**
 * @file WorkerPoolTest.cpp
 * @brief Unit tests for the FWorkerPool class.
 */

#include "Thread/WorkerPool.hpp"
#include "CoreTypes.hpp"

#include <gtest/gtest.h>

#include <thread>

TEST( ParallelWorkerPool, BasicSubmitAndWait )
{
    LumenEngine::Parallel::FWorkerPool Pool( 4U, 64ULL );
    LumenEngine::TAtomic<LumenEngine::Int32> Counter = 0;

    for ( LumenEngine::Int32 Index = 0; Index < 10; ++Index )
    {
        EXPECT_TRUE( Pool.Submit( [&Counter] () { Counter.fetch_add( 1, std::memory_order_relaxed ); } ) );
    }

    Pool.WaitAll();
    EXPECT_EQ( Counter.load(), 10 );
}

TEST( ParallelWorkerPool, QueueFullReturnsFalse )
{
    LumenEngine::Parallel::FWorkerPool Pool( 1U, 4ULL );

    LumenEngine::TAtomic<LumenEngine::Bool> bWorkerStarted = false;
    LumenEngine::TAtomic<LumenEngine::Bool> bBlockWorker   = true;

    /** INFO: Push a task that permanently blocks the sole worker */
    EXPECT_TRUE( Pool.Submit(
        [&bWorkerStarted, &bBlockWorker] ()
        {
            bWorkerStarted.store( true, std::memory_order_release );

            while ( bBlockWorker.load( std::memory_order_acquire ) )
            {
                std::this_thread::yield();
            }
        } ) );

    while ( not bWorkerStarted.load( std::memory_order_acquire ) )
    {
        std::this_thread::yield();
    }

    /** INFO: Fill the bounded queue's exact capacity (4) */
    EXPECT_TRUE( Pool.Submit( [] () {} ) );
    EXPECT_TRUE( Pool.Submit( [] () {} ) );
    EXPECT_TRUE( Pool.Submit( [] () {} ) );
    EXPECT_TRUE( Pool.Submit( [] () {} ) );

    /** INFO: The next push MUST fail because it overflows the capacity constraint */
    EXPECT_FALSE( Pool.Submit( [] () {} ) );

    /** INFO: Cleanup */
    bBlockWorker.store( false, std::memory_order_release );
    Pool.WaitAll();
}

TEST( ParallelWorkerPool, SubmitAfterShutdownReturnsFalse )
{
    LumenEngine::Parallel::FWorkerPool Pool( 2U, 16ULL );
    Pool.Shutdown();
    EXPECT_FALSE( Pool.Submit( [] () {} ) );
}

TEST( ParallelWorkerPool, MultiThreadedStress )
{
    LumenEngine::Parallel::FWorkerPool Pool( 8U, 2048ULL );
    LumenEngine::TAtomic<LumenEngine::Int32> Counter = 0;

    constexpr LumenEngine::Int32 NumTasks = 100000;

    for ( LumenEngine::Int32 Index = 0; Index < NumTasks; ++Index )
    {
        while ( not Pool.Submit( [&Counter] () { Counter.fetch_add( 1, std::memory_order_relaxed ); } ) )
        {
            std::this_thread::yield();
        }
    }

    Pool.WaitAll();
    EXPECT_EQ( Counter.load(), NumTasks );
}

TEST( ParallelWorkerPool, WorkerIndexContextIsExposedInsideTasks )
{
    constexpr LumenEngine::UInt32 WorkerCount = 4U;

    LumenEngine::Parallel::FWorkerPool Pool( WorkerCount, 64ULL );

    EXPECT_FALSE( LumenEngine::Parallel::FWorkerPool::GetCurrentWorkerIndex().has_value() );

    LumenEngine::TAtomic<LumenEngine::Bool> bAllTasksHadContext = true;
    LumenEngine::TAtomic<LumenEngine::Bool> bAllIndicesInRange  = true;

    constexpr LumenEngine::Int32 TaskCount = 64;
    for ( LumenEngine::Int32 Index = 0; Index < TaskCount; ++Index )
    {
        while ( not Pool.Submit(
            [&bAllTasksHadContext, &bAllIndicesInRange] ()
            {
                const LumenEngine::TOptional<LumenEngine::UInt32> WorkerIndex = LumenEngine::Parallel::FWorkerPool::GetCurrentWorkerIndex();

                if ( not WorkerIndex.has_value() )
                {
                    bAllTasksHadContext.store( false, std::memory_order_relaxed );
                    return;
                }

                if ( *WorkerIndex >= WorkerCount )
                {
                    bAllIndicesInRange.store( false, std::memory_order_relaxed );
                }
            } ) )
        {
            std::this_thread::yield();
        }
    }

    Pool.WaitAll();

    EXPECT_TRUE( bAllTasksHadContext.load( std::memory_order_relaxed ) );
    EXPECT_TRUE( bAllIndicesInRange.load( std::memory_order_relaxed ) );
    EXPECT_FALSE( LumenEngine::Parallel::FWorkerPool::GetCurrentWorkerIndex().has_value() );
}

TEST( ParallelWorkerPool, ExecutionCountersMatchExecutedTaskCount )
{
    LumenEngine::Parallel::FWorkerPool Pool( 2U, 128ULL );

    constexpr LumenEngine::UInt64 TaskCount = 200ULL;
    for ( LumenEngine::UInt64 Index = 0ULL; Index < TaskCount; ++Index )
    {
        while ( not Pool.Submit( [] () {} ) )
        {
            std::this_thread::yield();
        }
    }

    Pool.WaitAll();

    const LumenEngine::TVector<LumenEngine::UInt64> WorkerCounts = Pool.GetWorkerExecutionCounts();

    EXPECT_EQ( WorkerCounts.size(), 2ULL );

    LumenEngine::UInt64 Total = 0ULL;
    for ( const LumenEngine::UInt64 Count : WorkerCounts )
    {
        Total += Count;
    }

    EXPECT_EQ( Total, TaskCount );
    EXPECT_EQ( Pool.GetTotalExecutedTaskCount(), TaskCount );
}