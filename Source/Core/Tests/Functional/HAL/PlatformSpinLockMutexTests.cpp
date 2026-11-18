/**
 * @file FPlatformSpinLockMutexTests.cpp
 * @brief Functional contract tests for HAL::FPlatformSpinLockMutex.
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

static_assert( not std::is_copy_constructible_v<HAL::FPlatformSpinLockMutex> );
static_assert( not std::is_copy_assignable_v<HAL::FPlatformSpinLockMutex> );
static_assert( not std::is_move_constructible_v<HAL::FPlatformSpinLockMutex> );
static_assert( not std::is_move_assignable_v<HAL::FPlatformSpinLockMutex> );
static_assert( std::is_nothrow_default_constructible_v<HAL::FPlatformSpinLockMutex> );
static_assert( noexcept( std::declval<HAL::FPlatformSpinLockMutex &>().Lock() ) );
static_assert( noexcept( std::declval<HAL::FPlatformSpinLockMutex &>().Unlock() ) );
static_assert( noexcept( std::declval<HAL::FPlatformSpinLockMutex &>().TryLock() ) );
static_assert( noexcept( std::declval<HAL::FPlatformSpinLockMutex &>().IsLocked() ) );

/**
 * State
 */

LUMEN_TEST( PlatformSpinLock_State, DefaultUnlocked )
{
    HAL::FPlatformSpinLockMutex Lock;
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

/**
 * Lock / Unlock
 */

LUMEN_TEST( PlatformSpinLock_LockUnlock, BasicRoundTrip )
{
    HAL::FPlatformSpinLockMutex Lock;
    Lock.Lock();
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    Lock.Unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

LUMEN_TEST( PlatformSpinLock_LockUnlock, RepeatedCycles )
{
    HAL::FPlatformSpinLockMutex Lock;
    for ( Int32 Index = 0; Index < 1000; ++Index )
    {
        Lock.Lock();
        LUMEN_CHECK_TRUE( Lock.IsLocked() );
        Lock.Unlock();
        LUMEN_CHECK_FALSE( Lock.IsLocked() );
    }
}

/**
 * TryLock
 */

LUMEN_TEST( PlatformSpinLock_TryLock, SucceedsWhenFree )
{
    HAL::FPlatformSpinLockMutex Lock;
    LUMEN_CHECK_TRUE( Lock.TryLock() );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    Lock.Unlock();
}

LUMEN_TEST( PlatformSpinLock_TryLock, FailsWhenLocked )
{
    HAL::FPlatformSpinLockMutex Lock;
    Lock.Lock();
    LUMEN_CHECK_FALSE( Lock.TryLock() );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    Lock.Unlock();
}

LUMEN_TEST( PlatformSpinLock_TryLock, SucceedsAfterUnlock )
{
    HAL::FPlatformSpinLockMutex Lock;
    Lock.Lock();
    Lock.Unlock();
    LUMEN_CHECK_TRUE( Lock.TryLock() );
    Lock.Unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

/**
 * TLockGuard RAII
 */

LUMEN_TEST( PlatformSpinLock_Guard, LocksOnConstructionReleasesOnDestruction )
{
    HAL::FPlatformSpinLockMutex Lock;

    {
        HAL::TLockGuard<HAL::FPlatformSpinLockMutex> Guard( Lock );
        LUMEN_CHECK_TRUE( Lock.IsLocked() );
    }

    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

LUMEN_TEST( PlatformSpinLock_Guard, NestedOnDifferentLocks )
{
    HAL::FPlatformSpinLockMutex LockA;
    HAL::FPlatformSpinLockMutex LockB;

    {
        HAL::TLockGuard<HAL::FPlatformSpinLockMutex> GuardA( LockA );
        {
            HAL::TLockGuard<HAL::FPlatformSpinLockMutex> GuardB( LockB );
            LUMEN_CHECK_TRUE( LockA.IsLocked() );
            LUMEN_CHECK_TRUE( LockB.IsLocked() );
        }
        LUMEN_CHECK_TRUE( LockA.IsLocked() );
        LUMEN_CHECK_FALSE( LockB.IsLocked() );
    }

    LUMEN_CHECK_FALSE( LockA.IsLocked() );
    LUMEN_CHECK_FALSE( LockB.IsLocked() );
}

/**
 * Threading
 */

LUMEN_TEST( PlatformSpinLock_Threading, LockBlocksSecondThread )
{
    HAL::FPlatformSpinLockMutex Lock;
    TAtomic<Bool> bStarted{ false };
    TAtomic<Bool> bAcquired{ false };

    Lock.Lock();

    FThread Other( [&]
    {
        bStarted.store( true, std::memory_order::release );
        Lock.Lock();
        bAcquired.store( true, std::memory_order::release );
        Lock.Unlock();
    } );

    while ( not bStarted.load( std::memory_order::acquire ) )
    {
        HAL::FPlatformTime::Sleep( 1ms );
    }

    HAL::FPlatformTime::Sleep( 50ms );
    LUMEN_CHECK_FALSE( bAcquired.load( std::memory_order::acquire ) );

    Lock.Unlock();
    Other.Join();
    LUMEN_CHECK_TRUE( bAcquired.load( std::memory_order::acquire ) );
}

LUMEN_TEST( PlatformSpinLock_Threading, TryLockFailsFromOtherThreadWhileHeld )
{
    HAL::FPlatformSpinLockMutex Lock;
    TAtomic<Bool> bGot{ false };

    Lock.Lock();
    FThread Other( [&]
    {
        bGot.store( Lock.TryLock() );
        if ( bGot.load() )
        {
            Lock.Unlock();
        }
    } );
    Other.Join();

    LUMEN_CHECK_FALSE( bGot.load() );
    Lock.Unlock();
}

LUMEN_TEST( PlatformSpinLock_Threading, CounterMutationUnderLock )
{
    HAL::FPlatformSpinLockMutex Lock;
    Int32 Counter = 0;

    auto Work = [&]
    {
        for ( Int32 Index = 0; Index < 1000; ++Index )
        {
            HAL::TLockGuard<HAL::FPlatformSpinLockMutex> Guard( Lock );
            ++Counter;
        }
    };

    FThread T1( Work );
    FThread T2( Work );
    FThread T3( Work );
    T1.Join();
    T2.Join();
    T3.Join();

    LUMEN_CHECK_EQ( Counter, 3000 );
}

} // namespace Lumen
