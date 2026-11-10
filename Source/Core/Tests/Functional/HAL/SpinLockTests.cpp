/**
 * @file SpinLockTests.cpp
 * @brief Functional tests for HAL::FSpinLock and HAL::FSpinGuard.
 */

#include <HAL/SpinLock.hpp>
#include <Tests/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <type_traits>
#include <utility>

using namespace std::chrono_literals;

namespace Lumen
{

/**
 * API contracts (compile-time)
 */

static_assert( not std::is_copy_constructible_v<HAL::FSpinLock> and not std::is_copy_assignable_v<HAL::FSpinLock>,
               "FSpinLock must not be copyable" );
static_assert( not std::is_move_constructible_v<HAL::FSpinLock> and not std::is_move_assignable_v<HAL::FSpinLock>,
               "FSpinLock must not be movable" );
static_assert( std::is_nothrow_default_constructible_v<HAL::FSpinLock>, "FSpinLock must be noexcept default constructible" );
static_assert( noexcept( std::declval<HAL::FSpinLock &>().Lock() ), "FSpinLock::Lock must be noexcept" );
static_assert( noexcept( std::declval<HAL::FSpinLock &>().Unlock() ), "FSpinLock::Unlock must be noexcept" );
static_assert( noexcept( std::declval<HAL::FSpinLock &>().TryLock() ), "FSpinLock::TryLock must be noexcept" );
static_assert( noexcept( std::declval<HAL::FSpinLock &>().IsLocked() ), "FSpinLock::IsLocked must be noexcept" );

static_assert( not std::is_copy_constructible_v<HAL::FSpinGuard> and not std::is_copy_assignable_v<HAL::FSpinGuard>,
               "FSpinGuard must not be copyable" );
static_assert( not std::is_move_constructible_v<HAL::FSpinGuard> and not std::is_move_assignable_v<HAL::FSpinGuard>,
               "FSpinGuard must not be movable" );

/**
 * initial state
 */

LUMEN_TEST( SpinLock_State, DefaultUnlocked )
{
    HAL::FSpinLock Lock;

    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( static_cast<Bool>( Lock ) );
}

/**
 * Lock / Unlock
 */

LUMEN_TEST( SpinLock_LockUnlock, BasicRoundTrip )
{
    HAL::FSpinLock Lock;

    Lock.Lock();
    LUMEN_CHECK_TRUE( Lock.IsLocked() );

    Lock.Unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

LUMEN_TEST( SpinLock_LockUnlock, RepeatedCycles )
{
    HAL::FSpinLock Lock;

    for ( Int32 Index = 0; Index < 1000; ++Index )
    {
        Lock.Lock();
        LUMEN_CHECK_TRUE( Lock.IsLocked() );

        Lock.Unlock();
        LUMEN_CHECK_FALSE( Lock.IsLocked() );
    }
}

LUMEN_TEST( SpinLock_LockUnlock, OperatorBoolReflectsState )
{
    HAL::FSpinLock Lock;

    LUMEN_CHECK_FALSE( static_cast<Bool>( Lock ) );

    Lock.Lock();
    LUMEN_CHECK_TRUE( static_cast<Bool>( Lock ) );

    Lock.Unlock();
    LUMEN_CHECK_FALSE( static_cast<Bool>( Lock ) );
}

/**
 * TryLock
 */

LUMEN_TEST( SpinLock_TryLock, SucceedsWhenFree )
{
    HAL::FSpinLock Lock;

    LUMEN_CHECK_TRUE( Lock.TryLock() );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );

    Lock.Unlock();
}

LUMEN_TEST( SpinLock_TryLock, FailsWhenAlreadyLocked )
{
    HAL::FSpinLock Lock;

    Lock.Lock();
    LUMEN_CHECK_FALSE( Lock.TryLock() );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );

    Lock.Unlock();
}

LUMEN_TEST( SpinLock_TryLock, SucceedsAfterUnlock )
{
    HAL::FSpinLock Lock;

    Lock.Lock();
    Lock.Unlock();

    LUMEN_CHECK_TRUE( Lock.TryLock() );
    Lock.Unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

/**
 * FSpinLock IMutex compatibility surface
 */

LUMEN_TEST( SpinLock_IMutex, LowercaseLockUnlock )
{
    HAL::FSpinLock Lock;

    Lock.lock();
    LUMEN_CHECK_TRUE( Lock.IsLocked() );

    Lock.unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

LUMEN_TEST( SpinLock_IMutex, LowercaseTryLock )
{
    HAL::FSpinLock Lock;

    LUMEN_CHECK_TRUE( Lock.try_lock() );
    LUMEN_CHECK_FALSE( Lock.try_lock() );

    Lock.unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

LUMEN_TEST( SpinLock_IMutex, AccessThroughBaseInterface )
{
    HAL::FSpinLock Lock;
    Internal::IMutex &Interface = Lock;

    LUMEN_CHECK_FALSE( Interface.IsLocked() );

    Interface.Lock();
    LUMEN_CHECK_TRUE( Interface.IsLocked() );
    LUMEN_CHECK_FALSE( Interface.try_lock() );

    Interface.Unlock();
    LUMEN_CHECK_FALSE( Interface.IsLocked() );

    LUMEN_CHECK_TRUE( Interface.try_lock() );
    LUMEN_CHECK_TRUE( Interface.IsLocked() );

    Interface.Unlock();
    LUMEN_CHECK_FALSE( Interface.IsLocked() );
}

/**
 * FSpinGuard RAII
 */

LUMEN_TEST( SpinGuard_RAII, LocksOnConstructionReleasesOnDestruction )
{
    HAL::FSpinLock Lock;

    {
        HAL::FSpinGuard Guard( Lock );
        LUMEN_CHECK_TRUE( Lock.IsLocked() );
    }

    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

LUMEN_TEST( SpinGuard_RAII, NestedGuardsOnDifferentSpinLocks )
{
    HAL::FSpinLock LockA;
    HAL::FSpinLock LockB;

    {
        HAL::FSpinGuard GuardA( LockA );
        LUMEN_CHECK_TRUE( LockA.IsLocked() );

        {
            HAL::FSpinGuard GuardB( LockB );
            LUMEN_CHECK_TRUE( LockB.IsLocked() );
            LUMEN_CHECK_TRUE( LockA.IsLocked() );
        }

        LUMEN_CHECK_FALSE( LockB.IsLocked() );
        LUMEN_CHECK_TRUE( LockA.IsLocked() );
    }

    LUMEN_CHECK_FALSE( LockA.IsLocked() );
    LUMEN_CHECK_FALSE( LockB.IsLocked() );
}

LUMEN_TEST( SpinGuard_RAII, GuardAcquiresContendedLock )
{
    HAL::FSpinLock Lock;

    Lock.Lock();
    LUMEN_CHECK_TRUE( Lock.IsLocked() );

    /**
     * INFO: the guard must acquire a lock that was recently released,
     * including when the release happened right before construction
     */
    Lock.Unlock();

    {
        HAL::FSpinGuard Guard( Lock );
        LUMEN_CHECK_TRUE( Lock.IsLocked() );
    }

    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

/**
 * Multithreading
 */

LUMEN_TEST( SpinLock_Threading, LockBlocksSecondThread )
{
    HAL::FSpinLock Lock;
    std::atomic<Bool> SecondStarted{ false };
    std::atomic<Bool> SecondAcquired{ false };

    Lock.Lock();

    std::thread Other( [&] ()
    {
        SecondStarted.store( true, std::memory_order_release );
        Lock.Lock();
        SecondAcquired.store( true, std::memory_order_release );
        Lock.Unlock();
    } );

    while ( not SecondStarted.load( std::memory_order_acquire ) )
    {
        std::this_thread::sleep_for( 1ms );
    }

    std::this_thread::sleep_for( 50ms );
    const Bool WasAcquiredWhileLocked = SecondAcquired.load( std::memory_order_acquire );

    Lock.Unlock();
    Other.join();

    LUMEN_CHECK_FALSE( WasAcquiredWhileLocked );
    LUMEN_CHECK_TRUE( SecondAcquired.load( std::memory_order_acquire ) );
}

LUMEN_TEST( SpinLock_Threading, TryLockFailsFromOtherThreadWhileLocked )
{
    HAL::FSpinLock Lock;
    std::atomic<Bool> OtherObserved{ false };

    Lock.Lock();

    std::thread Other( [&] ()
    {
        OtherObserved.store( Lock.TryLock(), std::memory_order_release );
        Lock.Unlock();
    } );

    Other.join();

    LUMEN_CHECK_FALSE( OtherObserved.load( std::memory_order_acquire ) );
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

LUMEN_TEST( SpinLock_Threading, CounterMutationUnderLock )
{
    HAL::FSpinLock Lock;
    Int32 SharedCounter = 0;

    auto Increment = [&] ()
    {
        for ( Int32 Index = 0; Index < 1000; ++Index )
        {
            HAL::FSpinGuard Guard( Lock );
            ++SharedCounter;
        }
    };

    std::thread First( Increment );
    std::thread Second( Increment );
    std::thread Third( Increment );

    First.join();
    Second.join();
    Third.join();

    LUMEN_CHECK_EQ( SharedCounter, 3000 );
}

} // namespace Lumen
