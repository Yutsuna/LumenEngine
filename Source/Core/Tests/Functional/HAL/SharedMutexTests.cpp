/**
 * @file SharedMutexTests.cpp
 * @brief Functional tests for FSharedMutex, TSharedLock, TSharedUniqueLock.
 */

#include <Core/CoreDefinitions.hpp>

#include <HAL/SharedMutex.hpp>
#include <Tests/UnitTest.hpp>

#include <array>
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

static_assert( not std::is_copy_constructible_v<FSharedMutex> and not std::is_copy_assignable_v<FSharedMutex>,
               "FSharedMutex must not be copyable" );
static_assert( not std::is_move_constructible_v<FSharedMutex> and not std::is_move_assignable_v<FSharedMutex>,
               "FSharedMutex must not be movable" );
static_assert( std::is_nothrow_default_constructible_v<FSharedMutex>, "FSharedMutex must be noexcept default constructible" );
static_assert( noexcept( std::declval<FSharedMutex &>().Lock() ), "FSharedMutex::Lock must be noexcept" );
static_assert( noexcept( std::declval<FSharedMutex &>().Unlock() ), "FSharedMutex::Unlock must be noexcept" );
static_assert( noexcept( std::declval<FSharedMutex &>().LockShared() ), "FSharedMutex::LockShared must be noexcept" );
static_assert( noexcept( std::declval<FSharedMutex &>().UnlockShared() ), "FSharedMutex::UnlockShared must be noexcept" );

static_assert( not std::is_copy_constructible_v<TSharedLock<FSharedMutex>> and
                   not std::is_copy_assignable_v<TSharedLock<FSharedMutex>>,
               "TSharedLock must not be copyable" );
static_assert( not std::is_move_constructible_v<TSharedLock<FSharedMutex>> and
                   not std::is_move_assignable_v<TSharedLock<FSharedMutex>>,
               "TSharedLock must not be movable" );

static_assert( not std::is_copy_constructible_v<TSharedUniqueLock<FSharedMutex>> and
                   not std::is_copy_assignable_v<TSharedUniqueLock<FSharedMutex>>,
               "TSharedUniqueLock must not be copyable" );
static_assert( std::is_move_constructible_v<TSharedUniqueLock<FSharedMutex>> and
                   std::is_move_assignable_v<TSharedUniqueLock<FSharedMutex>>,
               "TSharedUniqueLock must be movable" );

/**
 * initial state
 */

LUMEN_TEST( SharedMutex_State, DefaultUnlocked )
{
    FSharedMutex Mutex;

    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );
    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 0U );
}

//
// exclusive lock
//

LUMEN_TEST( SharedMutex_Exclusive, LockUnlock )
{
    FSharedMutex Mutex;

    Mutex.Lock();
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );

    Mutex.Unlock();
    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );
}

LUMEN_TEST( SharedMutex_Exclusive, RepeatedLockUnlock )
{
    FSharedMutex Mutex;

    for ( Int32 Index = 0; Index < 100; ++Index )
    {
        Mutex.Lock();
        LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );

        Mutex.Unlock();
        LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );
    }
}

LUMEN_TEST( SharedMutex_Exclusive, TryLockSucceedsWhenFree )
{
    FSharedMutex Mutex;

    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );

    Mutex.Unlock();
    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );
}

LUMEN_TEST( SharedMutex_Exclusive, TryLockFailsWhenAlreadyLocked )
{
    FSharedMutex Mutex;

    Mutex.Lock();
    LUMEN_CHECK_FALSE( Mutex.TryLock() );

    Mutex.Unlock();
}

LUMEN_TEST( SharedMutex_Exclusive, TryLockFailsWhileReadersActive )
{
    FSharedMutex Mutex;

    Mutex.LockShared();
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
    Mutex.UnlockShared();

    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );
}

//
// shared lock
//

LUMEN_TEST( SharedMutex_Shared, SingleLockShared )
{
    FSharedMutex Mutex;

    Mutex.LockShared();
    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 1U );
    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );

    Mutex.UnlockShared();
    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 0U );
}

LUMEN_TEST( SharedMutex_Shared, MultipleLockSharedAccumulates )
{
    FSharedMutex Mutex;

    Mutex.LockShared();
    Mutex.LockShared();
    Mutex.LockShared();
    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 3U );

    Mutex.UnlockShared();
    Mutex.UnlockShared();
    Mutex.UnlockShared();
    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 0U );
}

LUMEN_TEST( SharedMutex_Shared, TryLockSharedSucceedsWhenFree )
{
    FSharedMutex Mutex;

    LUMEN_CHECK_TRUE( Mutex.TryLockShared() );
    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 1U );

    Mutex.UnlockShared();
}

LUMEN_TEST( SharedMutex_Shared, TryLockSharedFailsWhileWriterActive )
{
    FSharedMutex Mutex;

    Mutex.Lock();
    LUMEN_CHECK_FALSE( Mutex.TryLockShared() );
    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 0U );

    Mutex.Unlock();
}

//
// TSharedLock - RAII
//

LUMEN_TEST( SharedLock_RAII, LocksOnConstructionReleasesOnDestruction )
{
    FSharedMutex Mutex;

    {
        TSharedLock<FSharedMutex> Guard( Mutex );
        LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 1U );
        LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );
    }

    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 0U );
}

LUMEN_TEST( SharedLock_RAII, NestedGuards )
{
    FSharedMutex Mutex;

    {
        TSharedLock<FSharedMutex> Outer( Mutex );
        LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 1U );

        {
            TSharedLock<FSharedMutex> Inner( Mutex );
            LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 2U );
        }

        LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 1U );
    }

    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 0U );
}

LUMEN_TEST( SharedLock_RAII, BlocksWriterWhileHeld )
{
    FSharedMutex Mutex;

    TSharedLock<FSharedMutex> Guard( Mutex );
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
}

//
// construction
//

LUMEN_TEST( SharedUniqueLock, DefaultConstructionIsEmpty )
{
    TSharedUniqueLock<FSharedMutex> Lock;

    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( static_cast<Bool>( Lock ) );
}

LUMEN_TEST( SharedUniqueLock, ConstructAcquiresExclusive )
{
    FSharedMutex Mutex;

    TSharedUniqueLock<FSharedMutex> Lock( Mutex );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );
}

LUMEN_TEST( SharedUniqueLock, DestructorReleases )
{
    FSharedMutex Mutex;

    {
        TSharedUniqueLock<FSharedMutex> Lock( Mutex );
        LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );
    }

    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );
}

//
// manual control
//

LUMEN_TEST( SharedUniqueLock, ManualUnlock )
{
    FSharedMutex Mutex;

    TSharedUniqueLock<FSharedMutex> Lock( Mutex );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );

    Lock.Unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );
}

LUMEN_TEST( SharedUniqueLock, ManualLockAfterUnlock )
{
    FSharedMutex Mutex;

    TSharedUniqueLock<FSharedMutex> Lock( Mutex );
    Lock.Unlock();

    Lock.Lock();
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );
}

//
// TryLock
//

LUMEN_TEST( SharedUniqueLock, TryLockWithoutMutexFails )
{
    TSharedUniqueLock<FSharedMutex> Lock;

    LUMEN_CHECK_FALSE( Lock.TryLock() );
    LUMEN_CHECK_FALSE( Lock.IsLocked() );
}

LUMEN_TEST( SharedUniqueLock, TryLockSucceedsAfterManualUnlock )
{
    FSharedMutex Mutex;

    TSharedUniqueLock<FSharedMutex> Lock( Mutex );
    Lock.Unlock();

    LUMEN_CHECK_TRUE( Lock.TryLock() );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );
}

LUMEN_TEST( SharedUniqueLock, TryLockFailsWhileReadersActive )
{
    FSharedMutex Mutex;

    {
        TSharedLock<FSharedMutex> Reader( Mutex );

        TSharedUniqueLock<FSharedMutex> LUMEN_UNUSED Lock;
        LUMEN_CHECK_FALSE( Mutex.TryLock() );
    }

    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 0U );
}

//
// operator Bool
//

LUMEN_TEST( SharedUniqueLock, OperatorBool )
{
    FSharedMutex Mutex;

    TSharedUniqueLock<FSharedMutex> Lock( Mutex );
    LUMEN_CHECK_TRUE( static_cast<Bool>( Lock ) );

    Lock.Unlock();
    LUMEN_CHECK_FALSE( static_cast<Bool>( Lock ) );
}

//
// Multithreading - writer blocks readers
//

LUMEN_TEST( SharedMutex_Threading, WriterBlocksReader )
{
    FSharedMutex Mutex;
    TAtomic<Bool> WriterReady{ false };
    TAtomic<Bool> ReaderStarted{ false };
    TAtomic<Bool> ReaderAcquired{ false };
    TAtomic<Bool> WriterRelease{ false };

    std::thread Writer( [&] ()
    {
        Mutex.Lock();
        WriterReady.store( true, std::memory_order_release );
        while ( not WriterRelease.load( std::memory_order_acquire ) )
        {
            std::this_thread::sleep_for( 1ms );
        }
        Mutex.Unlock();
    } );

    while ( not WriterReady.load( std::memory_order_acquire ) )
    {
        std::this_thread::sleep_for( 1ms );
    }

    std::thread Reader( [&] ()
    {
        ReaderStarted.store( true, std::memory_order_release );
        Mutex.LockShared();
        ReaderAcquired.store( true, std::memory_order_release );
        Mutex.UnlockShared();
    } );

    while ( not ReaderStarted.load( std::memory_order_acquire ) )
    {
        std::this_thread::sleep_for( 1ms );
    }

    std::this_thread::sleep_for( 50ms );
    const Bool WasAcquiredDuringWriter = ReaderAcquired.load( std::memory_order_acquire );

    WriterRelease.store( true, std::memory_order_release );
    Writer.join();
    Reader.join();

    LUMEN_CHECK_FALSE( WasAcquiredDuringWriter );
    LUMEN_CHECK_TRUE( ReaderAcquired.load( std::memory_order_acquire ) );
}

/**
 * Multithreading - readers block writer
 */

LUMEN_TEST( SharedMutex_Threading, ReaderBlocksWriter )
{
    FSharedMutex Mutex;
    TAtomic<Bool> ReaderReady{ false };
    TAtomic<Bool> WriterStarted{ false };
    TAtomic<Bool> WriterAcquired{ false };
    TAtomic<Bool> ReaderRelease{ false };

    std::thread Reader( [&] ()
    {
        Mutex.LockShared();
        ReaderReady.store( true, std::memory_order_release );
        while ( not ReaderRelease.load( std::memory_order_acquire ) )
        {
            std::this_thread::sleep_for( 1ms );
        }
        Mutex.UnlockShared();
    } );

    while ( not ReaderReady.load( std::memory_order_acquire ) )
    {
        std::this_thread::sleep_for( 1ms );
    }

    std::thread Writer( [&] ()
    {
        WriterStarted.store( true, std::memory_order_release );
        Mutex.Lock();
        WriterAcquired.store( true, std::memory_order_release );
        Mutex.Unlock();
    } );

    while ( not WriterStarted.load( std::memory_order_acquire ) )
    {
        std::this_thread::sleep_for( 1ms );
    }

    std::this_thread::sleep_for( 50ms );
    const Bool WasWriterAcquiredDuringReader = WriterAcquired.load( std::memory_order_acquire );

    ReaderRelease.store( true, std::memory_order_release );
    Reader.join();
    Writer.join();

    LUMEN_CHECK_FALSE( WasWriterAcquiredDuringReader );
    LUMEN_CHECK_TRUE( WriterAcquired.load( std::memory_order_acquire ) );
}

/**
 * TSharedUniqueLock move semantics
 */

LUMEN_TEST( SharedUniqueLock, MoveConstructorTransfersOwnership )
{
    FSharedMutex Mutex;

    TSharedUniqueLock<FSharedMutex> Original( Mutex );
    TSharedUniqueLock<FSharedMutex> Moved( std::move( Original ) );

    LUMEN_CHECK_TRUE( Moved.IsLocked() );
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );

    LUMEN_CHECK_FALSE( Original.IsLocked() );
    LUMEN_CHECK_FALSE( static_cast<Bool>( Original ) );
    LUMEN_CHECK_TRUE( Original.Release() == nullptr );
}

LUMEN_TEST( SharedUniqueLock, MoveAssignmentReleasesHeldMutex )
{
    FSharedMutex MutexA;
    FSharedMutex MutexB;

    TSharedUniqueLock<FSharedMutex> Target( MutexA );
    TSharedUniqueLock<FSharedMutex> Source( MutexB );

    Target = std::move( Source );

    LUMEN_CHECK_FALSE( MutexA.IsWriteLocked() );
    LUMEN_CHECK_TRUE( MutexB.IsWriteLocked() );

    LUMEN_CHECK_TRUE( Target.IsLocked() );
    LUMEN_CHECK_TRUE( static_cast<Bool>( Target ) );

    LUMEN_CHECK_FALSE( Source.IsLocked() );
    LUMEN_CHECK_FALSE( static_cast<Bool>( Source ) );
}

LUMEN_TEST( SharedUniqueLock, MoveAssignmentFromEmptyReleasesOwnership )
{
    FSharedMutex Mutex;

    TSharedUniqueLock<FSharedMutex> Target( Mutex );
    TSharedUniqueLock<FSharedMutex> EmptySource;

    Target = std::move( EmptySource );

    LUMEN_CHECK_FALSE( Target.IsLocked() );
    LUMEN_CHECK_FALSE( static_cast<Bool>( Target ) );
    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );

    LUMEN_CHECK_FALSE( EmptySource.IsLocked() );
}

LUMEN_TEST( SharedUniqueLock, SelfMoveIsNoOp )
{
    FSharedMutex Mutex;

    TSharedUniqueLock<FSharedMutex> Lock( Mutex );
    TSharedUniqueLock<FSharedMutex> &SelfAlias = Lock;

    Lock = std::move( SelfAlias );

    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );
}

/**
 * TSharedUniqueLock Release
 */

LUMEN_TEST( SharedUniqueLock, ReleaseReturnsManagedMutex )
{
    FSharedMutex Mutex;

    TSharedUniqueLock<FSharedMutex> Lock( Mutex );

    FSharedMutex *Released = Lock.Release();

    LUMEN_CHECK_TRUE( Released == &Mutex );

    /** INFO: Release hands the lock over without unlocking it */
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );

    Lock.Unlock();
    LUMEN_CHECK_FALSE( Lock.IsLocked() );

    Mutex.Unlock();
    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );
}

LUMEN_TEST( SharedUniqueLock, DestructorAfterReleaseDoesNotTouchMutex )
{
    FSharedMutex Mutex;

    {
        TSharedUniqueLock<FSharedMutex> Lock( Mutex );
        LUMEN_CHECK_TRUE( Lock.Release() != nullptr );
        LUMEN_CHECK_FALSE( Lock.IsLocked() );
    }

    /** INFO: Ownership was detached, so the mutex must stay write locked */
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );

    Mutex.Unlock();
    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );
}

/**
 * Multithreading - several readers hold the shared lock at once
 */

LUMEN_TEST( SharedMutex_Threading, MultipleReadersShareSimultaneously )
{
    FSharedMutex Mutex;
    TAtomic<Bool> ReleaseReaders{ false };

    std::array<std::thread, 4> Readers;

    for ( std::thread &Reader : Readers )
    {
        Reader = std::thread( [&] ()
        {
            Mutex.LockShared();

            while ( not ReleaseReaders.load( std::memory_order_acquire ) )
            {
                std::this_thread::sleep_for( 1ms );
            }

            Mutex.UnlockShared();
        } );
    }

    USize WaitIterations = 0U;
    while ( Mutex.SharedLockCount() < 4U and WaitIterations < 2000U )
    {
        std::this_thread::sleep_for( 1ms );
        ++WaitIterations;
    }

    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 4U );
    LUMEN_CHECK_FALSE( Mutex.IsWriteLocked() );

    ReleaseReaders.store( true, std::memory_order_release );

    for ( std::thread &Reader : Readers )
    {
        Reader.join();
    }

    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 0U );
}

/**
 * Multithreading - a pending writer takes priority over new readers
 */

LUMEN_TEST( SharedMutex_Threading, PendingWriterBlocksNewSharedLock )
{
    FSharedMutex Mutex;
    TAtomic<Bool> ReleaseWriter{ false };
    TAtomic<Bool> WriterHolding{ false };
    TAtomic<Bool> ReaderAcquired{ false };

    Mutex.LockShared();

    std::thread Writer( [&] ()
    {
        /** INFO: Lock() sets WriterFlag immediately, then waits for readers to drain */
        Mutex.Lock();
        WriterHolding.store( true, std::memory_order_release );

        while ( not ReleaseWriter.load( std::memory_order_acquire ) )
        {
            std::this_thread::sleep_for( 1ms );
        }

        Mutex.Unlock();
    } );

    USize WriterFlagWaitIterations = 0U;
    while ( not Mutex.IsWriteLocked() and WriterFlagWaitIterations < 2000U )
    {
        std::this_thread::sleep_for( 1ms );
        ++WriterFlagWaitIterations;
    }
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );

    std::thread Reader( [&] ()
    {
        Mutex.LockShared();
        ReaderAcquired.store( true, std::memory_order_release );
        Mutex.UnlockShared();
    } );

    std::this_thread::sleep_for( 50ms );
    LUMEN_CHECK_FALSE( ReaderAcquired.load( std::memory_order_acquire ) );

    Mutex.UnlockShared();

    USize WriterWaitIterations = 0U;
    while ( not WriterHolding.load( std::memory_order_acquire ) and WriterWaitIterations < 2000U )
    {
        std::this_thread::sleep_for( 1ms );
        ++WriterWaitIterations;
    }
    LUMEN_CHECK_TRUE( WriterHolding.load( std::memory_order_acquire ) );

    /** INFO: The reader stays blocked while the writer owns the exclusive lock */
    LUMEN_CHECK_FALSE( ReaderAcquired.load( std::memory_order_acquire ) );

    ReleaseWriter.store( true, std::memory_order_release );
    Writer.join();

    USize ReaderWaitIterations = 0U;
    while ( not ReaderAcquired.load( std::memory_order_acquire ) and ReaderWaitIterations < 2000U )
    {
        std::this_thread::sleep_for( 1ms );
        ++ReaderWaitIterations;
    }
    LUMEN_CHECK_TRUE( ReaderAcquired.load( std::memory_order_acquire ) );
    Reader.join();

    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 0U );
}

/**
 * Multithreading - mixed readers and writers on shared state
 */

LUMEN_TEST( SharedMutex_Threading, MixedReadersWritersKeepSharedDataConsistent )
{
    FSharedMutex Mutex;
    Int32 SharedCounter = 0;
    TAtomic<Bool> OutOfRangeObserved{ false };

    auto WriterWork = [&] ()
    {
        for ( Int32 Index = 0; Index < 250; ++Index )
        {
            TSharedUniqueLock<FSharedMutex> WriterLock( Mutex );
            ++SharedCounter;
        }
    };

    auto ReaderWork = [&] ()
    {
        for ( Int32 Index = 0; Index < 250; ++Index )
        {
            TSharedLock<FSharedMutex> ReaderLock( Mutex );
            const Int32 Observed = SharedCounter;

            if ( Observed < 0 or Observed > 500 )
            {
                OutOfRangeObserved.store( true, std::memory_order_release );
            }
        }
    };

    std::thread WriterOne( WriterWork );
    std::thread WriterTwo( WriterWork );
    std::thread ReaderOne( ReaderWork );
    std::thread ReaderTwo( ReaderWork );

    WriterOne.join();
    WriterTwo.join();
    ReaderOne.join();
    ReaderTwo.join();

    LUMEN_CHECK_FALSE( OutOfRangeObserved.load( std::memory_order_acquire ) );
    LUMEN_CHECK_EQ( SharedCounter, 500 );
}

} // namespace Lumen
