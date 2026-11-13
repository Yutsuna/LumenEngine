/**
 * @file FPlatformSharedMutexTests.cpp
 * @brief Functional contract tests for HAL::FPlatformSharedMutex.
 */

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformMutex.hpp>
#include <HAL/PlatformTime.hpp>

#include <Templates/Array.hpp>

#include <Thread/Thread.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Compile-time API contracts
 */

static_assert( not std::is_copy_constructible_v<HAL::FPlatformSharedMutex> );
static_assert( not std::is_copy_assignable_v<HAL::FPlatformSharedMutex> );
static_assert( not std::is_move_constructible_v<HAL::FPlatformSharedMutex> );
static_assert( not std::is_move_assignable_v<HAL::FPlatformSharedMutex> );
static_assert( std::is_nothrow_default_constructible_v<HAL::FPlatformSharedMutex> );
static_assert( noexcept( std::declval<HAL::FPlatformSharedMutex &>().Lock() ) );
static_assert( noexcept( std::declval<HAL::FPlatformSharedMutex &>().Unlock() ) );
static_assert( noexcept( std::declval<HAL::FPlatformSharedMutex &>().LockShared() ) );
static_assert( noexcept( std::declval<HAL::FPlatformSharedMutex &>().UnlockShared() ) );
static_assert( noexcept( std::declval<HAL::FPlatformSharedMutex &>().TryLock() ) );
static_assert( noexcept( std::declval<HAL::FPlatformSharedMutex &>().TryLockShared() ) );

/** * Exclusive lock
 */

LUMEN_TEST( PlatformSharedMutex_Exclusive, LockUnlock )
{
    HAL::FPlatformSharedMutex Mutex;
    Mutex.Lock();
    Mutex.Unlock();
}

LUMEN_TEST( PlatformSharedMutex_Exclusive, TryLockSucceedsWhenFree )
{
    HAL::FPlatformSharedMutex Mutex;
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformSharedMutex_Exclusive, TryLockFailsWhenLocked )
{
    HAL::FPlatformSharedMutex Mutex;
    Mutex.Lock();
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformSharedMutex_Exclusive, TryLockFailsWhileReadersActive )
{
    HAL::FPlatformSharedMutex Mutex;
    Mutex.LockShared();
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
    Mutex.UnlockShared();
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

/** * Shared lock
 */

LUMEN_TEST( PlatformSharedMutex_Shared, LockSharedUnlockShared )
{
    HAL::FPlatformSharedMutex Mutex;
    Mutex.LockShared();
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
    Mutex.UnlockShared();
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformSharedMutex_Shared, TryLockSharedSucceedsWhenFree )
{
    HAL::FPlatformSharedMutex Mutex;
    LUMEN_CHECK_TRUE( Mutex.TryLockShared() );
    Mutex.UnlockShared();
}

LUMEN_TEST( PlatformSharedMutex_Shared, TryLockSharedFailsWhileWriterActive )
{
    HAL::FPlatformSharedMutex Mutex;
    Mutex.Lock();
    LUMEN_CHECK_FALSE( Mutex.TryLockShared() );
    Mutex.Unlock();
    LUMEN_CHECK_TRUE( Mutex.TryLockShared() );
    Mutex.UnlockShared();
}

/** * TSharedLock RAII
 */

LUMEN_TEST( PlatformSharedMutex_SharedLock, LocksOnConstructionReleasesOnDestruction )
{
    HAL::FPlatformSharedMutex Mutex;

    {
        HAL::TSharedLock<HAL::FPlatformSharedMutex> Guard( Mutex );
        LUMEN_CHECK_FALSE( Mutex.TryLock() );
    }

    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformSharedMutex_SharedLock, NestedOnDifferentMutexes )
{
    HAL::FPlatformSharedMutex MutexA;
    HAL::FPlatformSharedMutex MutexB;

    {
        HAL::TSharedLock<HAL::FPlatformSharedMutex> GuardA( MutexA );
        {
            HAL::TSharedLock<HAL::FPlatformSharedMutex> GuardB( MutexB );
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

/** * TSharedUniqueLock
 */

LUMEN_TEST( PlatformSharedMutex_SharedUniqueLock, ConstructAcquiresExclusive )
{
    HAL::FPlatformSharedMutex Mutex;
    HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
}

LUMEN_TEST( PlatformSharedMutex_SharedUniqueLock, DestructorReleases )
{
    HAL::FPlatformSharedMutex Mutex;

    {
        HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    }

    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

LUMEN_TEST( PlatformSharedMutex_SharedUniqueLock, MoveTransfersOwnership )
{
    HAL::FPlatformSharedMutex Mutex;
    HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Original( Mutex );
    HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Moved( std::move( Original ) );
    LUMEN_CHECK_TRUE( Moved.IsLocked() );
    LUMEN_CHECK_FALSE( Original.IsLocked() );
}

LUMEN_TEST( PlatformSharedMutex_SharedUniqueLock, ManualUnlockAndRelock )
{
    HAL::FPlatformSharedMutex Mutex;
    HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    Lock.Unlock();
    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
    LUMEN_CHECK_TRUE( Lock.TryLock() );
    LUMEN_CHECK_TRUE( Lock.IsLocked() );
}

/** * Threading — writer blocks reader
 */

LUMEN_TEST( PlatformSharedMutex_Threading, WriterBlocksReader )
{
    HAL::FPlatformSharedMutex Mutex;
    TAtomic<Bool> bWriterReady{ false };
    TAtomic<Bool> bReaderAcquired{ false };
    TAtomic<Bool> bRelease{ false };

    FThread Writer( [&]
    {
        Mutex.Lock();
        bWriterReady.store( true, std::memory_order::release );
        while ( not bRelease.load( std::memory_order::acquire ) )
        {
            HAL::FPlatformTime::Sleep( 1ms );
        }
        Mutex.Unlock();
    } );

    while ( not bWriterReady.load( std::memory_order::acquire ) )
    {
        HAL::FPlatformTime::Sleep( 1ms );
    }

    FThread Reader( [&]
    {
        Mutex.LockShared();
        bReaderAcquired.store( true, std::memory_order::release );
        Mutex.UnlockShared();
    } );

    HAL::FPlatformTime::Sleep( 50ms );
    LUMEN_CHECK_FALSE( bReaderAcquired.load( std::memory_order::acquire ) );

    bRelease.store( true, std::memory_order::release );
    Writer.Join();
    Reader.Join();
    LUMEN_CHECK_TRUE( bReaderAcquired.load( std::memory_order::acquire ) );
}

/** * Threading — reader blocks writer
 */

LUMEN_TEST( PlatformSharedMutex_Threading, ReaderBlocksWriter )
{
    HAL::FPlatformSharedMutex Mutex;
    TAtomic<Bool> bReaderReady{ false };
    TAtomic<Bool> bWriterAcquired{ false };
    TAtomic<Bool> bRelease{ false };

    FThread Reader( [&]
    {
        Mutex.LockShared();
        bReaderReady.store( true, std::memory_order::release );
        while ( not bRelease.load( std::memory_order::acquire ) )
        {
            HAL::FPlatformTime::Sleep( 1ms );
        }
        Mutex.UnlockShared();
    } );

    while ( not bReaderReady.load( std::memory_order::acquire ) )
    {
        HAL::FPlatformTime::Sleep( 1ms );
    }

    FThread Writer( [&]
    {
        Mutex.Lock();
        bWriterAcquired.store( true, std::memory_order::release );
        Mutex.Unlock();
    } );

    HAL::FPlatformTime::Sleep( 50ms );
    LUMEN_CHECK_FALSE( bWriterAcquired.load( std::memory_order::acquire ) );

    bRelease.store( true, std::memory_order::release );
    Reader.Join();
    Writer.Join();
    LUMEN_CHECK_TRUE( bWriterAcquired.load( std::memory_order::acquire ) );
}

/**
 * Threading — multiple readers hold the lock simultaneously
 */

LUMEN_TEST( PlatformSharedMutex_Threading, MultipleReadersShareSimultaneously )
{
    HAL::FPlatformSharedMutex Mutex;
    TAtomic<Bool> bRelease{ false };

    TArray<FThread, 4> Readers;

    for ( FThread &Reader : Readers )
    {
        Reader = FThread( [&]
        {
            Mutex.LockShared();
            while ( not bRelease.load( std::memory_order::acquire ) )
            {
                HAL::FPlatformTime::Sleep( 1ms );
            }
            Mutex.UnlockShared();
        } );
    }

    HAL::FPlatformTime::Sleep( 50ms );
    LUMEN_CHECK_FALSE( Mutex.TryLock() );

    bRelease.store( true, std::memory_order::release );

    for ( FThread &Reader : Readers )
    {
        Reader.Join();
    }

    LUMEN_CHECK_TRUE( Mutex.TryLock() );
    Mutex.Unlock();
}

/**
 * Threading — mixed readers and writers keep shared data consistent
 */

LUMEN_TEST( PlatformSharedMutex_Threading, MixedReadersWritersKeepDataConsistent )
{
    HAL::FPlatformSharedMutex Mutex;
    Int32 Counter = 0;
    TAtomic<Bool> bOutOfRange{ false };

    auto WriterWork = [&]
    {
        for ( Int32 Index = 0; Index < 250; ++Index )
        {
            HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( Mutex );
            ++Counter;
        }
    };

    auto ReaderWork = [&]
    {
        for ( Int32 Index = 0; Index < 250; ++Index )
        {
            HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
            const Int32 Observed = Counter;
            if ( Observed < 0 or Observed > 500 )
            {
                bOutOfRange.store( true );
            }
        }
    };

    FThread W1( WriterWork );
    FThread W2( WriterWork );
    FThread R1( ReaderWork );
    FThread R2( ReaderWork );

    W1.Join();
    W2.Join();
    R1.Join();
    R2.Join();

    LUMEN_CHECK_FALSE( bOutOfRange.load() );
    LUMEN_CHECK_EQ( Counter, 500 );
}

/** * Threading — pending writer blocks new readers
 */

LUMEN_TEST( PlatformSharedMutex_Threading, PendingWriterBlocksNewSharedLock )
{
    HAL::FPlatformSharedMutex Mutex;
    TAtomic<Bool> bReleaseWriter{ false };
    TAtomic<Bool> bWriterHolding{ false };
    TAtomic<Bool> bReaderAcquired{ false };

    Mutex.LockShared();

    FThread Writer( [&]
    {
        Mutex.Lock();
        bWriterHolding.store( true, std::memory_order::release );

        while ( not bReleaseWriter.load( std::memory_order::acquire ) )
        {
            HAL::FPlatformTime::Sleep( 1ms );
        }
        Mutex.Unlock();
    } );

    HAL::FPlatformTime::Sleep( 50ms );

    FThread Reader( [&]
    {
        Mutex.LockShared();
        bReaderAcquired.store( true, std::memory_order::release );
        Mutex.UnlockShared();
    } );

    HAL::FPlatformTime::Sleep( 50ms );
    LUMEN_CHECK_FALSE( bReaderAcquired.load( std::memory_order::acquire ) );

    Mutex.UnlockShared();

    bReleaseWriter.store( true, std::memory_order::release );
    Writer.Join();
    Reader.Join();

    LUMEN_CHECK_TRUE( bReaderAcquired.load( std::memory_order::acquire ) );
}

} // namespace Lumen
