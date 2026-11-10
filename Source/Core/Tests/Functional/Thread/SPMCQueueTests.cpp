/**
 * @file SPMCQueueTests.cpp
 * @brief Functional tests for Lumen::Thread::TSPMCQueue.
 */

#include <Tests/UnitTest.hpp>
#include <Thread/SPMCQueue.hpp>

#include <Core/CoreTypes.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <thread>
#include <vector>

namespace Lumen
{

/**
 * API contracts (compile-time)
 */

static_assert( not std::is_copy_constructible_v<Thread::TSPMCQueue<Int32>> and
                   not std::is_copy_assignable_v<Thread::TSPMCQueue<Int32>>,
               "TSPMCQueue must not be copyable" );
static_assert( not std::is_move_constructible_v<Thread::TSPMCQueue<Int32>> and
                   not std::is_move_assignable_v<Thread::TSPMCQueue<Int32>>,
               "TSPMCQueue must not be movable" );
static_assert( std::is_nothrow_destructible_v<Thread::TSPMCQueue<Int32>>, "TSPMCQueue must be noexcept destructible" );

namespace
{

    /**
     * @struct FCountedPayload
     * @brief Default-constructible payload tracking copies and moves for lifetime contract tests.
     */
    struct FCountedPayload
    {
        inline static Int32 Copies = 0;
        inline static Int32 Moves  = 0;

        Int32 Value = 0;

        FCountedPayload () noexcept = default;

        explicit FCountedPayload ( const Int32 InValue ) noexcept : Value( InValue )
        {
        }

        FCountedPayload ( const FCountedPayload &InOther ) noexcept : Value( InOther.Value )
        {
            ++Copies;
        }

        FCountedPayload ( FCountedPayload &&InOther ) noexcept : Value( InOther.Value )
        {
            InOther.Value = 0;
            ++Moves;
        }

        FCountedPayload &operator=( const FCountedPayload &InOther ) noexcept
        {
            Value = InOther.Value;
            ++Copies;
            return *this;
        }

        FCountedPayload &operator=( FCountedPayload &&InOther ) noexcept
        {
            Value         = InOther.Value;
            InOther.Value = 0;
            ++Moves;
            return *this;
        }
    };

    /**
     * @struct FOwnedPayload
     * @brief Move-only payload used to prove the move-push path never copies.
     */
    struct FOwnedPayload
    {
        TUniquePtr<UInt32> Handle = nullptr;

        FOwnedPayload () noexcept = default;

        explicit FOwnedPayload ( const UInt32 InValue ) noexcept
        {
            Handle = MakeUnique<UInt32>( InValue );
        }
    };

} // namespace

/**
 * Push - copy path
 */

LUMEN_TEST( SPMCQueue_Push, CopyPushStoresValue )
{
    Thread::TSPMCQueue<Int32> Queue( 4 );

    LUMEN_CHECK_TRUE( Queue.Push( 42 ) );

    const TOptional<Int32> Item = Queue.Pop();

    LUMEN_CHECK_HAS_VALUE( Item );
    LUMEN_CHECK_EQ( *Item, 42 );
}

LUMEN_TEST( SPMCQueue_Push, CopyPushPerformsExactlyOneCopy )
{
    Thread::TSPMCQueue<FCountedPayload> Queue( 4 );
    FCountedPayload::Copies = 0;
    FCountedPayload::Moves  = 0;

    const FCountedPayload Source( 7 );

    LUMEN_CHECK_TRUE( Queue.Push( Source ) );

    LUMEN_CHECK_EQ( FCountedPayload::Copies, 1 );
    LUMEN_CHECK_EQ( FCountedPayload::Moves, 0 );
}

LUMEN_TEST( SPMCQueue_Push, PushFailsWhenFullAndKeepsContent )
{
    Thread::TSPMCQueue<Int32> Queue( 4 );

    LUMEN_CHECK_TRUE( Queue.Push( 10 ) );
    LUMEN_CHECK_TRUE( Queue.Push( 20 ) );
    LUMEN_CHECK_TRUE( Queue.Push( 30 ) );
    LUMEN_CHECK_TRUE( Queue.Push( 40 ) );

    LUMEN_CHECK_FALSE( Queue.Push( 50 ) );

    /** INFO: The refused push must not have disturbed the enqueued content */
    LUMEN_CHECK_EQ( *Queue.Pop(), 10 );
    LUMEN_CHECK_EQ( *Queue.Pop(), 20 );
    LUMEN_CHECK_EQ( *Queue.Pop(), 30 );
    LUMEN_CHECK_EQ( *Queue.Pop(), 40 );

    /** INFO: After draining, the refused value fits again */
    LUMEN_CHECK_TRUE( Queue.Push( 50 ) );
    LUMEN_CHECK_EQ( *Queue.Pop(), 50 );
}

LUMEN_TEST( SPMCQueue_Push, MoveOnlyPayloadNeverCopied )
{
    Thread::TSPMCQueue<FOwnedPayload> Queue( 4 );

    LUMEN_CHECK_TRUE( Queue.Push( FOwnedPayload( 42U ) ) );

    const TOptional<FOwnedPayload> Item = Queue.Pop();

    LUMEN_CHECK_HAS_VALUE( Item );
    LUMEN_CHECK_TRUE( ( *Item ).Handle.Get() != nullptr );
    LUMEN_CHECK_EQ( *( *Item ).Handle, 42U );
}

/**
 * Push - move path
 */

LUMEN_TEST( SPMCQueue_Push, MovePushMovesOnceWithoutCopy )
{
    Thread::TSPMCQueue<FCountedPayload> Queue( 4 );
    FCountedPayload::Copies = 0;
    FCountedPayload::Moves  = 0;

    LUMEN_CHECK_TRUE( Queue.Push( FCountedPayload( 9 ) ) );

    LUMEN_CHECK_EQ( FCountedPayload::Moves, 1 );
    LUMEN_CHECK_EQ( FCountedPayload::Copies, 0 );
}

/**
 * Pop
 */

LUMEN_TEST( SPMCQueue_Pop, PopFromEmptyReturnsEmpty )
{
    Thread::TSPMCQueue<Int32> Queue( 4 );

    LUMEN_CHECK_FALSE( Queue.Pop().HasValue() );
}

LUMEN_TEST( SPMCQueue_Pop, PopFromRefilledEmptyQueueReturnsEmpty )
{
    Thread::TSPMCQueue<Int32> Queue( 4 );

    LUMEN_CHECK_TRUE( Queue.Push( 1 ) );
    LUMEN_CHECK_TRUE( Queue.Push( 2 ) );

    LUMEN_CHECK_HAS_VALUE( Queue.Pop() );
    LUMEN_CHECK_HAS_VALUE( Queue.Pop() );
    LUMEN_CHECK_FALSE( Queue.Pop().HasValue() );
}

LUMEN_TEST( SPMCQueue_Pop, PopMovesValueOutWithoutCopy )
{
    Thread::TSPMCQueue<FCountedPayload> Queue( 4 );
    FCountedPayload::Copies = 0;
    FCountedPayload::Moves  = 0;

    LUMEN_CHECK_TRUE( Queue.Push( FCountedPayload( 11 ) ) );

    const TOptional<FCountedPayload> Item = Queue.Pop();

    LUMEN_CHECK_HAS_VALUE( Item );
    LUMEN_CHECK_EQ( ( *Item ).Value, 11 );

    /** INFO: hand-off must move the payload out, never copy it */
    LUMEN_CHECK_EQ( FCountedPayload::Copies, 0 );
    LUMEN_CHECK_TRUE( FCountedPayload::Moves >= 2 );
}

LUMEN_TEST( SPMCQueue_Pop, PopDeliversFifoOrder )
{
    Thread::TSPMCQueue<Int32> Queue( 8 );

    for ( Int32 Value = 0; Value < 8; ++Value )
    {
        LUMEN_CHECK_TRUE( Queue.Push( Value ) );
    }

    for ( Int32 Value = 0; Value < 8; ++Value )
    {
        const TOptional<Int32> Item = Queue.Pop();

        LUMEN_CHECK_HAS_VALUE( Item );
        LUMEN_CHECK_EQ( *Item, Value );
    }
}

/**
 * Construction
 */

LUMEN_TEST( SPMCQueue_Construction, MinimumCapacityRoundTrip )
{
    Thread::TSPMCQueue<Int32> Queue( 2 );

    LUMEN_CHECK_TRUE( Queue.Push( 1 ) );
    LUMEN_CHECK_TRUE( Queue.Push( 2 ) );
    LUMEN_CHECK_FALSE( Queue.Push( 3 ) );

    LUMEN_CHECK_EQ( *Queue.Pop(), 1 );
    LUMEN_CHECK_EQ( *Queue.Pop(), 2 );
    LUMEN_CHECK_FALSE( Queue.Pop().HasValue() );
}

/**
 * Flow - interleaving, wraparound, refill
 */

LUMEN_TEST( SPMCQueue_Flow, InterleavedPushPopKeepsOrder )
{
    Thread::TSPMCQueue<Int32> Queue( 4 );
    Int32 Expected = 0;

    for ( Int32 Value = 0; Value < 6; ++Value )
    {
        LUMEN_CHECK_TRUE( Queue.Push( Value ) );

        if ( Value % 2 == 1 )
        {
            LUMEN_CHECK_EQ( *Queue.Pop(), Expected );
            ++Expected;
        }
    }

    for ( ;; )
    {
        const TOptional<Int32> Item = Queue.Pop();

        if ( not Item.HasValue() )
        {
            break;
        }

        LUMEN_CHECK_EQ( *Item, Expected );
        ++Expected;
    }

    LUMEN_CHECK_EQ( Expected, 6 );
}

LUMEN_TEST( SPMCQueue_Flow, WraparoundReusesSlots )
{
    Thread::TSPMCQueue<UInt64> Queue( 2 );

    for ( UInt64 Round = 0; Round < 16; ++Round )
    {
        const UInt64 Base = Round * 2ULL;

        LUMEN_CHECK_TRUE( Queue.Push( Base ) );
        LUMEN_CHECK_TRUE( Queue.Push( Base + 1ULL ) );

        LUMEN_CHECK_EQ( *Queue.Pop(), Base );
        LUMEN_CHECK_EQ( *Queue.Pop(), Base + 1ULL );
    }
}

LUMEN_TEST( SPMCQueue_Flow, RefillAfterFullDrain )
{
    Thread::TSPMCQueue<Int32> Queue( 4 );

    for ( Int32 Cycle = 0; Cycle < 8; ++Cycle )
    {
        for ( Int32 Index = 0; Index < 4; ++Index )
        {
            LUMEN_CHECK_TRUE( Queue.Push( Cycle * 4 + Index ) );
        }

        LUMEN_CHECK_FALSE( Queue.Push( -1 ) );

        for ( Int32 Index = 0; Index < 4; ++Index )
        {
            const TOptional<Int32> Item = Queue.Pop();

            LUMEN_CHECK_HAS_VALUE( Item );
            LUMEN_CHECK_EQ( *Item, Cycle * 4 + Index );
        }

        LUMEN_CHECK_FALSE( Queue.Pop().HasValue() );
    }
}

/**
 * Threading - single producer, multiple consumers
 */

LUMEN_TEST( SPMCQueue_Threading, MultiConsumersDeliverExactlyOnce )
{
    constexpr Int32 ItemCount     = 4000;
    constexpr Int32 ConsumerCount = 4;

    Thread::TSPMCQueue<Int32> Queue( 128 );
    std::array<std::vector<Int32>, ConsumerCount> ConsumedItems;
    std::atomic<Bool> bAllPushed{ false };

    auto Producer = [&] ()
    {
        for ( Int32 Value = 0; Value < ItemCount; ++Value )
        {
            while ( not Queue.Push( Value ) )
            {
                std::this_thread::yield();
            }
        }

        bAllPushed.store( true, std::memory_order_release );
    };

    auto Consumer = [&] ( const UInt32 ConsumerIndex )
    {
        std::vector<Int32> &Received = ConsumedItems[ConsumerIndex];

        for ( ;; )
        {
            if ( const TOptional<Int32> Item = Queue.Pop(); Item.HasValue() )
            {
                Received.push_back( *Item );
                continue;
            }

            if ( bAllPushed.load( std::memory_order_acquire ) )
            {
                break;
            }

            std::this_thread::yield();
        }
    };

    std::thread ProducerThread( Producer );
    std::array<std::thread, ConsumerCount> ConsumerThreads = { std::thread( Consumer, 0U ), std::thread( Consumer, 1U ),
                                                               std::thread( Consumer, 2U ), std::thread( Consumer, 3U ) };

    ProducerThread.join();

    for ( std::thread &ConsumerThread : ConsumerThreads )
    {
        ConsumerThread.join();
    }

    std::vector<Int32> MergedItems;

    for ( const std::vector<Int32> &Received : ConsumedItems )
    {
        MergedItems.insert( MergedItems.end(), Received.begin(), Received.end() );
    }

    LUMEN_CHECK_EQ( static_cast<Int32>( MergedItems.size() ), ItemCount );

    std::sort( MergedItems.begin(), MergedItems.end() );

    for ( Int32 Index = 0; Index < ItemCount; ++Index )
    {
        if ( MergedItems[static_cast<USize>( Index )] != Index ) [[unlikely]]
        {
            LUMEN_CHECK_MSG( false, "every pushed value must be consumed exactly once, in the multiset sense" );
            break;
        }
    }
}

LUMEN_TEST( SPMCQueue_Threading, SingleConsumerPreservesFifoUnderLoad )
{
    constexpr Int32 ItemCount = 1000;

    Thread::TSPMCQueue<UInt64> Queue( 64 );
    std::vector<UInt64> ReceivedItems;
    std::atomic<Bool> bAllPushed{ false };

    auto Producer = [&] ()
    {
        for ( UInt64 Value = 0; Value < static_cast<UInt64>( ItemCount ); ++Value )
        {
            while ( not Queue.Push( Value ) )
            {
                std::this_thread::yield();
            }
        }

        bAllPushed.store( true, std::memory_order_release );
    };

    std::thread ProducerThread( Producer );

    for ( ;; )
    {
        if ( const TOptional<UInt64> Item = Queue.Pop(); Item.HasValue() )
        {
            ReceivedItems.push_back( *Item );
            continue;
        }

        if ( bAllPushed.load( std::memory_order_acquire ) )
        {
            break;
        }

        std::this_thread::yield();
    }

    ProducerThread.join();

    LUMEN_CHECK_EQ( static_cast<Int32>( ReceivedItems.size() ), ItemCount );

    for ( Int32 Index = 0; Index < ItemCount; ++Index )
    {
        if ( ReceivedItems[static_cast<USize>( Index )] != static_cast<UInt64>( Index ) ) [[unlikely]]
        {
            LUMEN_CHECK_MSG( false, "a single consumer must observe strictly increasing FIFO values" );
            break;
        }
    }
}

LUMEN_TEST( SPMCQueue_Threading, ProducerAndConsumerCoexistWithoutDeadlock )
{
    constexpr Int32 ItemCount = 500;

    Thread::TSPMCQueue<Int32> Queue( 2 );
    std::atomic<Int32> PoppedCount{ 0 };
    std::atomic<Bool> bAllPushed{ false };

    auto Producer = [&] ()
    {
        for ( Int32 Value = 0; Value < ItemCount; ++Value )
        {
            while ( not Queue.Push( Value ) )
            {
                std::this_thread::yield();
            }
        }

        bAllPushed.store( true, std::memory_order_release );
    };

    auto Consumer = [&] ()
    {
        for ( ;; )
        {
            if ( const TOptional<Int32> Item = Queue.Pop(); Item.HasValue() )
            {
                PoppedCount.fetch_add( 1, std::memory_order_relaxed );
                continue;
            }

            if ( bAllPushed.load( std::memory_order_acquire ) )
            {
                break;
            }

            std::this_thread::yield();
        }
    };

    std::thread ProducerThread( Producer );
    std::thread ConsumerThread( Consumer );

    ProducerThread.join();
    ConsumerThread.join();

    LUMEN_CHECK_EQ( PoppedCount.load( std::memory_order_relaxed ), ItemCount );
    LUMEN_CHECK_FALSE( Queue.Pop().HasValue() );
}

} // namespace Lumen
