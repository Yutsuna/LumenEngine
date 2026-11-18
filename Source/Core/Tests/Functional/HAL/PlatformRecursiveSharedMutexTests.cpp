/**
 * @file FPlatformRecursiveSharedMutexTests.cpp
 * @brief Functional contract tests for HAL::FPlatformRecursiveSharedMutex.
 */

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformMutex.hpp>
#include <HAL/PlatformTime.hpp>

#include <Thread/Thread.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Compile-time API contracts
 */

static_assert( not std::is_copy_constructible_v<HAL::FPlatformRecursiveSharedMutex> );
static_assert( not std::is_move_constructible_v<HAL::FPlatformRecursiveSharedMutex> );
static_assert( std::is_nothrow_default_constructible_v<HAL::FPlatformRecursiveSharedMutex> );
static_assert( noexcept( std::declval<HAL::FPlatformRecursiveSharedMutex &>().Lock() ) );
static_assert( noexcept( std::declval<HAL::FPlatformRecursiveSharedMutex &>().Unlock() ) );
static_assert( noexcept( std::declval<HAL::FPlatformRecursiveSharedMutex &>().TryLock() ) );

/**
 * Basic lock / unlock
 */

LUMEN_TEST( PlatformRecursiveMutex_Basic, LockUnlock )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    Mutex.Lock();
    Mutex.Unlock();
}

LUMEN_TEST( PlatformRecursiveMutex_Basic, RepeatedCycles )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    for ( Int32 Index = 0; Index < 100; ++Index )
    {
        Mutex.Lock();
        Mutex.Unlock();
    }
}

/**
 * Recursive locking
 */

LUMEN_TEST( PlatformRecursiveMutex_Recursion, DoubleLockDoesNotDeadlock )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    Mutex.Lock();
    Mutex.Lock();
    Mutex.Unlock();
    Mutex.Unlock();
}

LUMEN_TEST( PlatformRecursiveMutex_Recursion, DeepRecursion )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    for ( Int32 Level = 0; Level < 10; ++Level )
    {
        Mutex.Lock();
    }
    for ( Int32 Level = 0; Level < 10; ++Level )
    {
        Mutex.Unlock();
    }
}

LUMEN_TEST( PlatformRecursiveMutex_Recursion, TryLockFromSameThreadSucceeds )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    Mutex.Lock();
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
    Mutex.Unlock();
}

LUMEN_TEST( PlatformRecursiveMutex_Recursion, TryLockDeepFromSameThreadSucceeds )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    Mutex.Lock();
    Mutex.Lock();
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
    Mutex.Unlock();
    Mutex.Unlock();
}

/**
 * Recursion depth is honoured: partial unlock keeps the mutex held
 */

LUMEN_TEST( PlatformRecursiveMutex_Recursion, PartialUnlockKeepsMutexHeld )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    Mutex.Lock();
    Mutex.Lock();
    Mutex.Unlock();

    TAtomic<Bool> bGot{ false };
    FThread Other( [&]
    {
        bGot.store( Mutex.TryLock() );
        if ( bGot.load() )
        {
            Mutex.Unlock();
        }
    } );
    Other.Join();

    LUMEN_CHECK_FALSE( bGot.load() );

    Mutex.Unlock();
}

LUMEN_TEST( PlatformRecursiveMutex_Recursion, FullUnlockReleasesMutex )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    Mutex.Lock();
    Mutex.Lock();
    Mutex.Unlock();
    Mutex.Unlock();

    TAtomic<Bool> bGot{ false };
    FThread Other( [&]
    {
        bGot.store( Mutex.TryLock() );
        if ( bGot.load() )
        {
            Mutex.Unlock();
        }
    } );
    Other.Join();

    LUMEN_CHECK_TRUE( bGot.load() );
}

/**
 * TryLock from another thread while held
 */

LUMEN_TEST( PlatformRecursiveMutex_TryLock, FailsFromOtherThreadWhileHeld )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    Mutex.Lock();

    TAtomic<Bool> bGot{ false };
    FThread Other( [&]
    {
        bGot.store( Mutex.TryLock() );
        if ( bGot.load() )
        {
            Mutex.Unlock();
        }
    } );
    Other.Join();

    LUMEN_CHECK_FALSE( bGot.load() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformRecursiveMutex_TryLock, SucceedsFromOtherThreadWhenFree )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;

    TAtomic<Bool> bGot{ false };
    FThread Other( [&]
    {
        bGot.store( Mutex.TryLock() );
        if ( bGot.load() )
        {
            Mutex.Unlock();
        }
    } );
    Other.Join();

    LUMEN_CHECK_TRUE( bGot.load() );
}

LUMEN_TEST( PlatformRecursiveMutex_TryLock, FailsFromOtherThreadAfterPartialUnlock )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    Mutex.Lock();
    Mutex.Lock();
    Mutex.Unlock();

    TAtomic<Bool> bGot{ false };
    FThread Other( [&]
    {
        bGot.store( Mutex.TryLock() );
        if ( bGot.load() )
        {
            Mutex.Unlock();
        }
    } );
    Other.Join();

    LUMEN_CHECK_FALSE( bGot.load() );
    Mutex.Unlock();
}

/**
 * Threading: mutual exclusion
 */

LUMEN_TEST( PlatformRecursiveMutex_Threading, LockBlocksOtherThread )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    TAtomic<Bool> bStarted{ false };
    TAtomic<Bool> bAcquired{ false };

    Mutex.Lock();

    FThread Other( [&]
    {
        bStarted.store( true, std::memory_order::release );
        Mutex.Lock();
        bAcquired.store( true, std::memory_order::release );
        Mutex.Unlock();
    } );

    while ( not bStarted.load( std::memory_order::acquire ) )
    {
        HAL::FPlatformTime::Sleep( 1ms );
    }

    HAL::FPlatformTime::Sleep( 50ms );
    LUMEN_CHECK_FALSE( bAcquired.load( std::memory_order::acquire ) );

    Mutex.Unlock();
    Other.Join();
    LUMEN_CHECK_TRUE( bAcquired.load( std::memory_order::acquire ) );
}

LUMEN_TEST( PlatformRecursiveMutex_Threading, RecursiveLockBlocksOtherThreadUntilFullyUnlocked )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    TAtomic<Bool> bStarted{ false };
    TAtomic<Bool> bAcquired{ false };

    Mutex.Lock();
    Mutex.Lock();

    FThread Other( [&]
    {
        bStarted.store( true, std::memory_order::release );
        Mutex.Lock();
        bAcquired.store( true, std::memory_order::release );
        Mutex.Unlock();
    } );

    while ( not bStarted.load( std::memory_order::acquire ) )
    {
        HAL::FPlatformTime::Sleep( 1ms );
    }

    Mutex.Unlock();
    HAL::FPlatformTime::Sleep( 50ms );
    LUMEN_CHECK_FALSE( bAcquired.load( std::memory_order::acquire ) );

    Mutex.Unlock();
    Other.Join();
    LUMEN_CHECK_TRUE( bAcquired.load( std::memory_order::acquire ) );
}

LUMEN_TEST( PlatformRecursiveMutex_Threading, CounterMutationUnderLock )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    Int32 Counter = 0;

    auto Work = [&]
    {
        for ( Int32 Index = 0; Index < 1000; ++Index )
        {
            Mutex.Lock();
            ++Counter;
            Mutex.Unlock();
        }
    };

    FThread T1( Work );
    FThread T2( Work );
    T1.Join();
    T2.Join();

    LUMEN_CHECK_EQ( Counter, 2000 );
}

LUMEN_TEST( PlatformRecursiveMutex_Threading, RecursiveCounterMutation )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;
    Int32 Counter = 0;

    auto Work = [&]
    {
        for ( Int32 Index = 0; Index < 500; ++Index )
        {
            Mutex.Lock();
            Mutex.Lock();
            ++Counter;
            Mutex.Unlock();
            Mutex.Unlock();
        }
    };

    FThread T1( Work );
    FThread T2( Work );
    T1.Join();
    T2.Join();

    LUMEN_CHECK_EQ( Counter, 1000 );
}

/**
 * RAII compatibility — nested guards on a recursive mutex must not deadlock
 */

LUMEN_TEST( PlatformRecursiveMutex_LockGuard, NestedGuardsOnSameMutex )
{
    HAL::FPlatformRecursiveSharedMutex Mutex;

    {
        HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Outer( Mutex );

        {
            HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Inner( Mutex );
        }
    }

    LUMEN_CHECK_TRUE( true );
}

} // namespace Lumen
