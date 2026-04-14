/**
 * @file MutexTest.cpp
 * @brief Unit tests for FMutex in LumenEngine
 */

#include "HAL/Mutex.hpp"
#include "CoreTypes.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <barrier>
#include <chrono>
#include <thread>
#include <vector>

TEST( ParallelMutex, InitialStateIsUnlocked )
{
    LumenEngine::FMutex Mutex;

    EXPECT_FALSE( Mutex.IsLocked() );
}

TEST( ParallelMutex, TryLockAndUnlockTransitionsState )
{
    LumenEngine::FMutex Mutex;

    EXPECT_TRUE( Mutex.TryLock() );
    EXPECT_TRUE( Mutex.IsLocked() );

    EXPECT_FALSE( Mutex.TryLock() );

    Mutex.Unlock();
    EXPECT_FALSE( Mutex.IsLocked() );
}

TEST( ParallelMutex, LockAndUnlockAllowMutualExclusion )
{
    LumenEngine::FMutex Mutex;
    LumenEngine::TAtomic<LumenEngine::Int32> ProtectedCounter = 0;

    constexpr LumenEngine::Int32 ThreadCount         = 8;
    constexpr LumenEngine::Int32 IncrementsPerThread = 5000;

    std::barrier StartBarrier( ThreadCount );
    std::vector<std::jthread> Workers;
    Workers.reserve( ThreadCount );

    for ( LumenEngine::Int32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex )
    {
        Workers.emplace_back(
            [&Mutex, &ProtectedCounter, &StartBarrier] ()
            {
                StartBarrier.arrive_and_wait();

                for ( LumenEngine::Int32 Iteration = 0; Iteration < IncrementsPerThread; ++Iteration )
                {
                    Mutex.Lock();
                    ++ProtectedCounter;
                    Mutex.Unlock();
                }
            } );
    }

    Workers.clear();

    const LumenEngine::Int32 Expected = ThreadCount * IncrementsPerThread;
    EXPECT_EQ( ProtectedCounter.load(), Expected );
}

TEST( ParallelMutex, TryLockFailsWhenHeldByAnotherThread )
{
    LumenEngine::FMutex Mutex;
    std::barrier SyncPoint( 2 );
    LumenEngine::TAtomic<LumenEngine::Bool> bObservedTryLockFailure = false;

    Mutex.Lock();

    std::jthread Worker(
        [&] ()
        {
            SyncPoint.arrive_and_wait();
            bObservedTryLockFailure = not Mutex.TryLock();
        } );

    SyncPoint.arrive_and_wait();
    Mutex.Unlock();

    EXPECT_TRUE( bObservedTryLockFailure.load() );
}

TEST( ParallelMutex, LockBlocksUntilAnotherThreadUnlocks )
{
    LumenEngine::FMutex Mutex;
    LumenEngine::TAtomic<LumenEngine::Bool> bSecondThreadEnteredCriticalSection = false;
    std::barrier SyncPoint( 2 );

    Mutex.Lock();

    std::jthread Worker(
        [&] ()
        {
            SyncPoint.arrive_and_wait();
            Mutex.Lock();
            bSecondThreadEnteredCriticalSection.store( true, std::memory_order_release );
            Mutex.Unlock();
        } );

    SyncPoint.arrive_and_wait();
    std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
    EXPECT_FALSE( bSecondThreadEnteredCriticalSection.load( std::memory_order_acquire ) );

    Mutex.Unlock();
    Worker.join();

    EXPECT_TRUE( bSecondThreadEnteredCriticalSection.load( std::memory_order_acquire ) );
}
