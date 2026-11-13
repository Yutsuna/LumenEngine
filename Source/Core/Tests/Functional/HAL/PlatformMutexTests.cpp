/**
 * @file PlatformMutexTests.cpp
 * @brief Functional contract tests for HAL::FPlatformMutex (non-recursive mutex).
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

static_assert( not std::is_copy_constructible_v<HAL::FPlatformMutex> );
static_assert( not std::is_copy_assignable_v<HAL::FPlatformMutex> );
static_assert( not std::is_move_constructible_v<HAL::FPlatformMutex> );
static_assert( not std::is_move_assignable_v<HAL::FPlatformMutex> );
static_assert( std::is_nothrow_default_constructible_v<HAL::FPlatformMutex> );
static_assert( std::is_nothrow_destructible_v<HAL::FPlatformMutex> );
static_assert( noexcept( std::declval<HAL::FPlatformMutex &>().Lock() ) );
static_assert( noexcept( std::declval<HAL::FPlatformMutex &>().Unlock() ) );
static_assert( noexcept( std::declval<HAL::FPlatformMutex &>().TryLock() ) );

/**
 * Lock / Unlock — basic contract
 */

LUMEN_TEST( PlatformMutex_LockUnlock, BasicRoundTrip )
{
    HAL::FPlatformMutex Mutex;
    Mutex.Lock();
    Mutex.Unlock();
    LUMEN_CHECK_TRUE( true );
}

LUMEN_TEST( PlatformMutex_LockUnlock, RepeatedCycles )
{
    HAL::FPlatformMutex Mutex;
    for ( Int32 Index = 0; Index < 100; ++Index )
    {
        Mutex.Lock();
        Mutex.Unlock();
    }
    LUMEN_CHECK_TRUE( true );
}

/**
 * TryLock — non-recursive mutex must return false from the owning thread
 */

LUMEN_TEST( PlatformMutex_TryLock, SucceedsWhenFree )
{
    HAL::FPlatformMutex Mutex;
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformMutex_TryLock, FailsWhenLockedBySameThread )
{
    HAL::FPlatformMutex Mutex;
    Mutex.Lock();
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformMutex_TryLock, SucceedsAfterUnlock )
{
    HAL::FPlatformMutex Mutex;
    Mutex.Lock();
    Mutex.Unlock();
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

/**
 * TLockGuard
 */

LUMEN_TEST( PlatformMutex_LockGuard, LocksOnConstructionReleasesOnDestruction )
{
    HAL::FPlatformMutex Mutex;

    {
        HAL::TLockGuard<HAL::FPlatformMutex> Guard( Mutex );
        LUMEN_CHECK_FALSE( Mutex.TryLock() );
    }

    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformMutex_LockGuard, NestedOnDifferentMutexes )
{
    HAL::FPlatformMutex MutexA;
    HAL::FPlatformMutex MutexB;

    {
        HAL::TLockGuard<HAL::FPlatformMutex> GuardA( MutexA );
        {
            HAL::TLockGuard<HAL::FPlatformMutex> GuardB( MutexB );
            LUMEN_CHECK_FALSE( MutexA.TryLock() );
            LUMEN_CHECK_FALSE( MutexB.TryLock() );
        }
        LUMEN_CHECK_FALSE( MutexA.TryLock() );
        LUMEN_CHECK_TRUE( MutexB.TryLock() );
        MutexB.Unlock();
    }

    LUMEN_CHECK_TRUE( MutexA.TryLock() );
    MutexA.Unlock();
}

/**
 * TUniqueLock — constructors
 */

LUMEN_TEST( PlatformMutex_UniqueLock, DefaultConstructionIsEmpty )
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock;
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( static_cast<Bool>( Lock ) );
}

LUMEN_TEST( PlatformMutex_UniqueLock, ConstructAcquires )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
}

LUMEN_TEST( PlatformMutex_UniqueLock, DestructorReleases )
{
    HAL::FPlatformMutex Mutex;

    {
        HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    }

    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformMutex_UniqueLock, DeferLockDoesNotAcquire )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex, HAL::FDeferLockT );
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformMutex_UniqueLock, TryToLockAcquiresWhenFree )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex, HAL::FTryToLockT );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
}

LUMEN_TEST( PlatformMutex_UniqueLock, TryToLockFailsWhenHeld )
{
    HAL::FPlatformMutex Mutex;
    Mutex.Lock();
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex, HAL::FTryToLockT );
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformMutex_UniqueLock, AdoptLockAssumesOwnership )
{
    HAL::FPlatformMutex Mutex;
    Mutex.Lock();
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex, HAL::FAdoptLockT );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
    Lock.Unlock();
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

/**
 * TUniqueLock — manual control
 */

LUMEN_TEST( PlatformMutex_UniqueLock, ManualUnlock )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    Lock.Unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformMutex_UniqueLock, ManualLockAfterUnlock )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    Lock.Unlock();
    Lock.Lock();
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
}

LUMEN_TEST( PlatformMutex_UniqueLock, LockOnNullIsNoOp )
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock;
    Lock.Lock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    Lock.Unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

/**
 * TUniqueLock — TryLock
 */

LUMEN_TEST( PlatformMutex_UniqueLock, TryLockWithoutMutexFails )
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock;
    LUMEN_CHECK_FALSE( Lock.TryLock() );
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

LUMEN_TEST( PlatformMutex_UniqueLock, TryLockSucceedsAfterManualUnlock )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    Lock.Unlock();
    LUMEN_CHECK_TRUE( Lock.TryLock() );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
}

LUMEN_TEST( PlatformMutex_UniqueLock, TryLockFailsWhenAlreadyHeld )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    LUMEN_CHECK_FALSE( Lock.TryLock() );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    Lock.Unlock();
}

/**
 * TUniqueLock — operator Bool
 */

LUMEN_TEST( PlatformMutex_UniqueLock, OperatorBool )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    LUMEN_CHECK_TRUE( static_cast<Bool>( Lock ) );
    Lock.Unlock();
    LUMEN_CHECK_FALSE( static_cast<Bool>( Lock ) );
}

/**
 * TUniqueLock — move semantics
 */

LUMEN_TEST( PlatformMutex_UniqueLock, MoveConstructorTransfersOwnership )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Original( Mutex );
    HAL::TUniqueLock<HAL::FPlatformMutex> Moved( std::move( Original ) );

    LUMEN_CHECK_TRUE( Moved.IsLocked() );
    LUMEN_CHECK_FALSE( Original.IsLocked() );
    LUMEN_CHECK_TRUE( Original.Release() == nullptr );
}

LUMEN_TEST( PlatformMutex_UniqueLock, MoveAssignmentReleasesHeldMutex )
{
    HAL::FPlatformMutex MutexA;
    HAL::FPlatformMutex MutexB;
    HAL::TUniqueLock<HAL::FPlatformMutex> Target( MutexA );
    HAL::TUniqueLock<HAL::FPlatformMutex> Source( MutexB );

    Target = std::move( Source );

    LUMEN_CHECK_TRUE( Target.IsLocked() );
    LUMEN_CHECK_FALSE( Source.IsLocked() );
    LUMEN_CHECK_TRUE( MutexA.TryLock() );
    MutexA.Unlock();
}

LUMEN_TEST( PlatformMutex_UniqueLock, SelfMoveIsNoOp )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    HAL::TUniqueLock<HAL::FPlatformMutex> &Alias = Lock;
    Lock                                         = std::move( Alias );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
}

/**
 * TUniqueLock — Release
 */

LUMEN_TEST( PlatformMutex_UniqueLock, ReleaseReturnsManagedMutex )
{
    HAL::FPlatformMutex Mutex;
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    HAL::FPlatformMutex *Released = Lock.Release();

    LUMEN_CHECK_TRUE( Released == &Mutex );
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformMutex_UniqueLock, ReleaseWithoutMutexReturnsNull )
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock;
    LUMEN_CHECK_TRUE( Lock.Release() == nullptr );
}

/**
 * Threading
 */

LUMEN_TEST( PlatformMutex_Threading, LockBlocksSecondThread )
{
    HAL::FPlatformMutex Mutex;
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

LUMEN_TEST( PlatformMutex_Threading, TryLockFailsFromOtherThreadWhileHeld )
{
    HAL::FPlatformMutex Mutex;
    TAtomic<Bool> bGot{ false };

    Mutex.Lock();
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

LUMEN_TEST( PlatformMutex_Threading, CounterMutationUnderLock )
{
    HAL::FPlatformMutex Mutex;
    Int32 Counter = 0;

    auto Work = [&]
    {
        for ( Int32 Index = 0; Index < 1000; ++Index )
        {
            HAL::TLockGuard<HAL::FPlatformMutex> Guard( Mutex );
            ++Counter;
        }
    };

    FThread T1( Work );
    FThread T2( Work );
    T1.Join();
    T2.Join();

    LUMEN_CHECK_EQ( Counter, 2000 );
}

LUMEN_TEST( PlatformMutex_Threading, UniqueLockCounterMutation )
{
    HAL::FPlatformMutex Mutex;
    Int32 Counter = 0;

    auto Work = [&]
    {
        for ( Int32 Index = 0; Index < 1000; ++Index )
        {
            HAL::TUniqueLock<HAL::FPlatformMutex> Owner( Mutex );
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
