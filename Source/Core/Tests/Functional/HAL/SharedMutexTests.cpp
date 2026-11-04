/// @file SharedMutexTests.cpp
/// @brief Unit and concurrency tests for FSharedMutex.

#include <HAL/Mutex.hpp>
#include <HAL/SharedMutex.hpp>
#include <Tests/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <vector>

namespace Lumen
{

LUMEN_TEST( SharedMutex_Locking, TryLockSharedObservesReadersAndWriter )
{
    FSharedMutex Mutex;

    LUMEN_CHECK_TRUE( Mutex.TryLockShared() );
    LUMEN_CHECK_EQ( Mutex.SharedLockCount(), 1U );
    LUMEN_CHECK_FALSE( Mutex.TryLock() );
    Mutex.UnlockShared();

    TUniqueLock<FSharedMutex> Writer( Mutex );
    LUMEN_CHECK_TRUE( Mutex.IsWriteLocked() );
    LUMEN_CHECK_FALSE( Mutex.TryLockShared() );
}

LUMEN_TEST( SharedMutex_Concurrency, ReadersAndWritersPublishProtectedWrites )
{
    constexpr UInt32 WriterCount    = 2U;
    constexpr UInt32 ReaderCount    = 4U;
    constexpr UInt32 IterationCount = 2000U;

    FSharedMutex Mutex;
    UInt64 FirstValue  = 0ULL;
    UInt64 SecondValue = 0ULL;
    TAtomic<Bool> bStart{ false };
    TAtomic<Bool> bFailure{ false };
    std::vector<std::thread> Threads;
    Threads.reserve(WriterCount + ReaderCount);

    for ( UInt32 Writer = 0U; Writer < WriterCount; ++Writer )
    {
        Threads.emplace_back(
            [&]
            {
                while ( not bStart.load( std::memory_order_acquire ) )
                {
                    std::this_thread::yield();
                }

                for ( UInt32 Index = 0U; Index < IterationCount; ++Index )
                {
                    TUniqueLock<FSharedMutex> Lock( Mutex );
                    ++FirstValue;
                    SecondValue = FirstValue;
                }
            } );
    }

    for ( UInt32 Reader = 0U; Reader < ReaderCount; ++Reader )
    {
        Threads.emplace_back(
            [&]
            {
                while ( not bStart.load( std::memory_order_acquire ) )
                {
                    std::this_thread::yield();
                }

                for ( UInt32 Index = 0U; Index < IterationCount; ++Index )
                {
                    TSharedLock<FSharedMutex> Lock( Mutex );
                    if ( FirstValue != SecondValue )
                    {
                        bFailure.store( true, std::memory_order_release );
                    }
                }
            } );
    }

    bStart.store( true, std::memory_order_release );
    for ( std::thread &Thread : Threads )
    {
        Thread.join();
    }

    LUMEN_CHECK_FALSE( bFailure.load() );
    LUMEN_CHECK_EQ( FirstValue, WriterCount * IterationCount );
    LUMEN_CHECK_EQ( SecondValue, FirstValue );
}

} // namespace Lumen
