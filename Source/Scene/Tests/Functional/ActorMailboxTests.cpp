/// @file ActorMailboxTests.cpp
/// @brief Unit and concurrency tests for the actor MPSC mailbox.

#include <Actor/ActorMailbox.hpp>
#include <Tests/UnitTest.hpp>

#include <array>
#include <atomic>
#include <thread>
#include <vector>

namespace Lumen
{

struct FMailboxTestPayload
{
    UInt32 Producer = 0U;
    UInt32 Sequence = 0U;
};

template <> struct TMessageTraits<FMailboxTestPayload>
{
    static constexpr Bool bEnabled = true;
};

namespace
{

    FMessage MakeMailboxMessage ( const UInt32 InProducer, const UInt32 InSequence )
    {
        return FMessage::Make( 1U, ActorID{}, FMailboxTestPayload{ InProducer, InSequence } );
    }

} // namespace

LUMEN_TEST( ActorMailbox_Queue, PreservesMessagesAcrossPoolFallback )
{
    FMailBox Mailbox( 0ULL, 2ULL );

    for ( UInt32 Index = 0U; Index < 8U; ++Index )
    {
        LUMEN_CHECK_EQ( Mailbox.Push( MakeMailboxMessage( 0U, Index ) ), EEnqueueResult::Success );
    }

    for ( UInt32 Index = 0U; Index < 8U; ++Index )
    {
        TOptional<FMessage> Message = Mailbox.Pop();
        LUMEN_CHECK_HAS_VALUE( Message );
        LUMEN_CHECK_EQ( ( *Message ).GetPayload<FMailboxTestPayload>( 1U ).Sequence, Index );
    }

    LUMEN_CHECK_FALSE( Mailbox.Pop().HasValue() );
    Mailbox.Close();
}

LUMEN_TEST( ActorMailbox_Pool, AllowsSynchronizedProducerThreadHandoffs )
{
    FMailBox Mailbox( 0ULL, 2ULL );

    LUMEN_CHECK_EQ( Mailbox.Push( MakeMailboxMessage( 0U, 1U ) ), EEnqueueResult::Success );
    LUMEN_CHECK_EQ( Mailbox.Push( MakeMailboxMessage( 0U, 2U ) ), EEnqueueResult::Success );

    UInt32 FirstSequence  = 0U;
    UInt32 SecondSequence = 0U;

    std::thread Consumer(
        [&]
        {
            TOptional<FMessage> First  = Mailbox.Pop();
            TOptional<FMessage> Second = Mailbox.Pop();

            if ( First.HasValue() )
            {
                FirstSequence = ( *First ).GetPayload<FMailboxTestPayload>( 1U ).Sequence;
            }

            if ( Second.HasValue() )
            {
                SecondSequence = ( *Second ).GetPayload<FMailboxTestPayload>( 1U ).Sequence;
            }
        } );

    Consumer.join();

    LUMEN_CHECK_EQ( FirstSequence, 1U );
    LUMEN_CHECK_EQ( SecondSequence, 2U );
    LUMEN_CHECK_EQ( Mailbox.Push( MakeMailboxMessage( 0U, 3U ) ), EEnqueueResult::Success );

    TOptional<FMessage> Third = Mailbox.Pop();

    LUMEN_CHECK_HAS_VALUE( Third );
    LUMEN_CHECK_EQ( ( *Third ).GetPayload<FMailboxTestPayload>( 1U ).Sequence, 3U );

    Mailbox.Close();
}

LUMEN_TEST( ActorMailbox_Backpressure, RejectsWhenFullAndRecoversAfterPop )
{
    FMailBox Mailbox( 1ULL, 1ULL );

    LUMEN_CHECK_EQ( Mailbox.Push( MakeMailboxMessage( 0U, 1U ) ), EEnqueueResult::Success );
    LUMEN_CHECK_EQ( Mailbox.Push( MakeMailboxMessage( 0U, 2U ) ), EEnqueueResult::Full );
    LUMEN_CHECK_HAS_VALUE( Mailbox.Pop() );
    LUMEN_CHECK_EQ( Mailbox.Push( MakeMailboxMessage( 0U, 3U ) ), EEnqueueResult::Success );

    Mailbox.Close();
    LUMEN_CHECK_TRUE( Mailbox.IsClosed() );
    LUMEN_CHECK_EQ( Mailbox.Push( MakeMailboxMessage( 0U, 4U ) ), EEnqueueResult::Closed );
    LUMEN_CHECK_FALSE( Mailbox.Pop().HasValue() );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
    Mailbox.Close();
}

LUMEN_TEST( ActorMailbox_Close, CoordinatesReserveWithCleanup )
{
    constexpr UInt32 IterationCount = 16U;

    for ( UInt32 Iteration = 0U; Iteration < IterationCount; ++Iteration )
    {
        FMailBox Mailbox;
        std::thread Reserver( [&] { Mailbox.Reserve( FMailBox::PoolCapacity ); } );
        std::thread Closer( [&] { Mailbox.Close(); } );

        Reserver.join();
        Closer.join();
        LUMEN_CHECK_TRUE( Mailbox.IsClosed() );
        Mailbox.Reserve( FMailBox::PoolCapacity );
    }
}

LUMEN_TEST( ActorMailbox_Close, CoordinatesIsEmptyWithCleanup )
{
    constexpr UInt32 IterationCount = 16U;

    for ( UInt32 Iteration = 0U; Iteration < IterationCount; ++Iteration )
    {
        FMailBox Mailbox;
        LUMEN_CHECK_EQ( Mailbox.Push( MakeMailboxMessage( 0U, Iteration ) ), EEnqueueResult::Success );

        TAtomic<Bool> bObserverStopped{ false };
        std::thread Observer(
            [&]
            {
                while ( not Mailbox.IsClosed() )
                {
                    ( void )Mailbox.IsEmpty();
                }
                bObserverStopped.store( true, std::memory_order_release );
            } );

        Mailbox.Close();
        Observer.join();

        LUMEN_CHECK_TRUE( bObserverStopped.load() );
        LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
    }
}

LUMEN_TEST( ActorMailbox_MPSC, ConcurrentProducersDeliverEveryMessageOnce )
{
    constexpr UInt32 ProducerCount       = 4U;
    constexpr UInt32 MessagesPerProducer = 2000U;
    constexpr UInt32 TotalMessages       = ProducerCount * MessagesPerProducer;

    FMailBox Mailbox( 0ULL, FMailBox::PoolCapacity );
    TAtomic<Bool> bStart{ false };
    TAtomic<UInt32> FinishedProducers{ 0U };
    TAtomic<Bool> bProducerFailure{ false };
    std::vector<std::thread> Producers;

    for ( UInt32 Producer = 0U; Producer < ProducerCount; ++Producer )
    {
        Producers.emplace_back(
            [&, Producer]
            {
                while ( not bStart.load( std::memory_order_acquire ) )
                {
                    std::this_thread::yield();
                }

                for ( UInt32 Sequence = 0U; Sequence < MessagesPerProducer; ++Sequence )
                {
                    if ( Mailbox.Push( MakeMailboxMessage( Producer, Sequence ) ) != EEnqueueResult::Success )
                    {
                        bProducerFailure.store( true, std::memory_order_release );
                        break;
                    }
                }
                FinishedProducers.fetch_add( 1U, std::memory_order_release );
            } );
    }

    std::array<std::array<Bool, MessagesPerProducer>, ProducerCount> Seen = {};
    UInt32 ReceivedCount                                                  = 0U;
    bStart.store( true, std::memory_order_release );

    while ( ReceivedCount < TotalMessages )
    {
        TOptional<FMessage> Message = Mailbox.Pop();
        if ( Message.HasValue() )
        {
            const FMailboxTestPayload Payload = ( *Message ).GetPayload<FMailboxTestPayload>( 1U );
            if ( Payload.Producer >= ProducerCount || Payload.Sequence >= MessagesPerProducer ||
                 Seen[Payload.Producer][Payload.Sequence] )
            {
                bProducerFailure.store( true, std::memory_order_release );
                break;
            }

            Seen[Payload.Producer][Payload.Sequence] = true;
            ++ReceivedCount;
        }
        else
        {
            if ( bProducerFailure.load( std::memory_order_acquire ) ||
                 ( FinishedProducers.load( std::memory_order_acquire ) == ProducerCount and Mailbox.IsEmpty() ) )
            {
                break;
            }
            std::this_thread::yield();
        }
    }

    for ( std::thread &Producer : Producers )
    {
        Producer.join();
    }

    LUMEN_CHECK_FALSE( bProducerFailure.load() );
    LUMEN_CHECK_EQ( ReceivedCount, TotalMessages );
    Mailbox.Close();
}

} // namespace Lumen
