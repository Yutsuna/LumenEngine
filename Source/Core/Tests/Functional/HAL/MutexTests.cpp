/// @file MutexTests.cpp
/// @brief Functional tests for FMutex, TLockGuard, TUniqueLock.

#include <HAL/Mutex.hpp>
#include <Tests/UnitTest.hpp>

#include <Core/CoreDefinitions.hpp>

#include <atomic>
#include <thread>

using namespace std::chrono_literals;

namespace Lumen
{

///
/// initial state
///

LUMEN_TEST( Mutex_State, DefaultUnlocked )
{
    FMutex Mutex;

    LUMEN_CHECK_FALSE( Mutex.IsLocked() );
    LUMEN_CHECK_FALSE( static_cast<Bool>( Mutex ) );
}

///
/// Lock / Unlock
///

LUMEN_TEST( Mutex_LockUnlock, BasicRoundTrip )
{
    FMutex Mutex;

    Mutex.Lock();
    LUMEN_CHECK_TRUE( Mutex.IsLocked() );

    Mutex.Unlock();
    LUMEN_CHECK_FALSE( Mutex.IsLocked() );
}

LUMEN_TEST( Mutex_LockUnlock, RepeatedCycles )
{
    FMutex Mutex;

    for ( Int32 Index = 0; Index < 100; ++Index )
    {
        Mutex.Lock();
        LUMEN_CHECK_TRUE( Mutex.IsLocked() );

        Mutex.Unlock();
        LUMEN_CHECK_FALSE( Mutex.IsLocked() );
    }
}

LUMEN_TEST( Mutex_LockUnlock, OperatorBoolReflectsState )
{
    FMutex Mutex;

    LUMEN_CHECK_FALSE( static_cast<Bool>( Mutex ) );

    Mutex.Lock();
    LUMEN_CHECK_TRUE( static_cast<Bool>( Mutex ) );

    Mutex.Unlock();
    LUMEN_CHECK_FALSE( static_cast<Bool>( Mutex ) );
}

///
/// TryLock
///

LUMEN_TEST( Mutex_TryLock, SucceedsWhenFree )
{
    FMutex Mutex;

    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    LUMEN_CHECK_TRUE( Mutex.IsLocked() );

    Mutex.Unlock();
}

LUMEN_TEST( Mutex_TryLock, FailsWhenAlreadyLocked )
{
    FMutex Mutex;

    Mutex.Lock();
    LUMEN_CHECK_FALSE( Mutex.TryLock() );

    Mutex.Unlock();
}

LUMEN_TEST( Mutex_TryLock, SucceedsAfterUnlock )
{
    FMutex Mutex;

    Mutex.Lock();
    Mutex.Unlock();

    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

///
/// FMutex IMutex lowercase compatibility surface
///

LUMEN_TEST( Mutex_IMutex, LowercaseLockUnlock )
{
    FMutex Mutex;

    Mutex.lock();
    LUMEN_CHECK_TRUE( Mutex.IsLocked() );

    Mutex.unlock();
    LUMEN_CHECK_FALSE( Mutex.IsLocked() );
}

LUMEN_TEST( Mutex_IMutex, LowercaseTryLock )
{
    FMutex Mutex;

    LUMEN_CHECK_TRUE( Mutex.try_lock() );
    LUMEN_CHECK_FALSE( Mutex.try_lock() );

    Mutex.unlock();
    LUMEN_CHECK_FALSE( Mutex.IsLocked() );
}

///
/// TLockGuard RAII
///

LUMEN_TEST( LockGuard_RAII, LocksOnConstructionReleasesOnDestruction )
{
    FMutex Mutex;

    {
        TLockGuard<FMutex> Guard( Mutex );
        LUMEN_CHECK_TRUE( Mutex.IsLocked() );
    }

    LUMEN_CHECK_FALSE( Mutex.IsLocked() );
}

LUMEN_TEST( LockGuard_RAII, NestedGuardsOnDifferentMutexes )
{
    FMutex MutexA;
    FMutex MutexB;

    {
        TLockGuard<FMutex> GuardA( MutexA );
        LUMEN_CHECK_TRUE( MutexA.IsLocked() );

        {
            TLockGuard<FMutex> GuardB( MutexB );
            LUMEN_CHECK_TRUE( MutexB.IsLocked() );
        }

        LUMEN_CHECK_TRUE( MutexA.IsLocked() );
        LUMEN_CHECK_FALSE( MutexB.IsLocked() );
    }

    LUMEN_CHECK_FALSE( MutexA.IsLocked() );
    LUMEN_CHECK_FALSE( MutexB.IsLocked() );
}

///
/// TUniqueLock construction
///

LUMEN_TEST( UniqueLock, DefaultConstructionIsEmpty )
{
    TUniqueLock<FMutex> Lock;

    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( static_cast<Bool>( Lock ) );
}

LUMEN_TEST( UniqueLock, ConstructAcquires )
{
    FMutex Mutex;

    TUniqueLock<FMutex> Lock( Mutex );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_TRUE( Mutex.IsLocked() );
}

LUMEN_TEST( UniqueLock, DestructorReleases )
{
    FMutex Mutex;

    {
        TUniqueLock<FMutex> Lock( Mutex );
        LUMEN_CHECK_TRUE( Mutex.IsLocked() );
    }

    LUMEN_CHECK_FALSE( Mutex.IsLocked() );
}

///
/// TUniqueLock manual control
///

LUMEN_TEST( UniqueLock, ManualUnlock )
{
    FMutex Mutex;

    TUniqueLock<FMutex> Lock( Mutex );
    Lock.Unlock();

    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( Mutex.IsLocked() );
}

LUMEN_TEST( UniqueLock, ManualLockAfterUnlock )
{
    FMutex Mutex;

    TUniqueLock<FMutex> Lock( Mutex );
    Lock.Unlock();

    Lock.Lock();
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_TRUE( Mutex.IsLocked() );
}

LUMEN_TEST( UniqueLock, LockOnNullIsNoOp )
{
    TUniqueLock<FMutex> Lock;

    Lock.Lock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );

    Lock.Unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

///
/// TUniqueLock TryLock
///

LUMEN_TEST( UniqueLock, TryLockWithoutMutexFails )
{
    TUniqueLock<FMutex> Lock;

    LUMEN_CHECK_FALSE( Lock.TryLock() );
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

LUMEN_TEST( UniqueLock, TryLockSucceedsAfterManualUnlock )
{
    FMutex Mutex;

    TUniqueLock<FMutex> Lock( Mutex );
    Lock.Unlock();

    LUMEN_CHECK_TRUE( Lock.TryLock() );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_TRUE( Mutex.IsLocked() );
}

LUMEN_TEST( UniqueLock, TryLockFailsWhenAlreadyHeld )
{
    FMutex Mutex;

    TUniqueLock<FMutex> Lock( Mutex );

    LUMEN_CHECK_FALSE( Lock.TryLock() );
    LUMEN_CHECK_FALSE( Lock.IsLocked() );

    Mutex.Unlock();
}

///
/// TUniqueLock operator Bool
///

LUMEN_TEST( UniqueLock, OperatorBool )
{
    FMutex Mutex;

    TUniqueLock<FMutex> Lock( Mutex );
    LUMEN_CHECK_TRUE( static_cast<Bool>( Lock ) );

    Lock.Unlock();
    LUMEN_CHECK_FALSE( static_cast<Bool>( Lock ) );
}

///
/// Multithreading
///

LUMEN_TEST( Mutex_Threading, LockBlocksSecondThread )
{
    FMutex Mutex;
    std::atomic<Bool> SecondStarted{ false };
    std::atomic<Bool> SecondAcquired{ false };

    Mutex.Lock();

    std::thread Other(
        [&] ()
        {
            SecondStarted.store( true, std::memory_order_release );
            Mutex.Lock();
            SecondAcquired.store( true, std::memory_order_release );
            Mutex.Unlock();
        } );

    while ( !SecondStarted.load( std::memory_order_acquire ) )
    {
        std::this_thread::sleep_for( 1ms );
    }

    std::this_thread::sleep_for( 50ms );
    const Bool WasAcquiredWhileLocked = SecondAcquired.load( std::memory_order_acquire );

    Mutex.Unlock();
    Other.join();

    LUMEN_CHECK_FALSE( WasAcquiredWhileLocked );
    LUMEN_CHECK_TRUE( SecondAcquired.load( std::memory_order_acquire ) );
}

LUMEN_TEST( Mutex_Threading, CounterMutationUnderLock )
{
    FMutex Mutex;
    Int32 SharedCounter = 0;

    auto Increment = [&] ()
    {
        for ( Int32 Index = 0; Index < 1000; ++Index )
        {
            TLockGuard<FMutex> Guard( Mutex );
            ++SharedCounter;
        }
    };

    std::thread T1( Increment );
    std::thread T2( Increment );

    T1.join();
    T2.join();

    LUMEN_CHECK_EQ( SharedCounter, 2000 );
}

} // namespace Lumen
