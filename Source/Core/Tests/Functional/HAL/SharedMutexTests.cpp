/// @file SharedMutexTests.cpp
/// @brief Functional tests for FSharedMutex, TSharedLock, TSharedUniqueLock.

#include <Core/CoreDefinitions.hpp>

#include <HAL/SharedMutex.hpp>
#include <Tests/UnitTest.hpp>

#include <atomic>
#include <thread>

using namespace std::chrono_literals;

namespace Lumen
{

///
/// initial state
///

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

    std::thread Writer(
        [&] ()
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

    std::thread Reader(
        [&] ()
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

///
/// Multithreading - readers block writer
///

LUMEN_TEST( SharedMutex_Threading, ReaderBlocksWriter )
{
    FSharedMutex Mutex;
    TAtomic<Bool> ReaderReady{ false };
    TAtomic<Bool> WriterStarted{ false };
    TAtomic<Bool> WriterAcquired{ false };
    TAtomic<Bool> ReaderRelease{ false };

    std::thread Reader(
        [&] ()
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

    std::thread Writer(
        [&] ()
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

} // namespace Lumen
