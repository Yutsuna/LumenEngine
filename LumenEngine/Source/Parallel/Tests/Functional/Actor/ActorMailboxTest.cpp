#include "Actor/ActorMailbox.hpp"
#include "Container/Vector.hpp"

#include <gtest/gtest.h>

#include <barrier>
#include <thread>
#include <unordered_set>

namespace
{

struct FValuePayload final
{
    LumenEngine::Int32 Value = 0;
};

struct FDataPayload final
{
    LumenEngine::Float32 X, Y, Z, W;
    LumenEngine::Int32 ID;
};

} // namespace

TEST( ParallelMailbox, EmptyInitially )
{
    LumenEngine::FMailBox Mailbox;

    EXPECT_TRUE( Mailbox.IsEmpty() );
    EXPECT_FALSE( Mailbox.Pop().has_value() );
}

TEST( ParallelMailbox, PushThenPopSingleMessage )
{
    LumenEngine::FMailBox Mailbox;

    FValuePayload Payload;
    Payload.Value = 1337;

    Mailbox.Push( LumenEngine::FMessage::Make( 42U, 7ULL, Payload ) );

    ASSERT_FALSE( Mailbox.IsEmpty() );

    LumenEngine::TOptional<LumenEngine::FMessage> Popped = Mailbox.Pop();
    ASSERT_TRUE( Popped.has_value() );
    EXPECT_EQ( Popped->Type, 42U );
    EXPECT_EQ( Popped->Sender, 7ULL );

    const FValuePayload &PoppedPayload = Popped->GetPayload<FValuePayload>();
    EXPECT_EQ( PoppedPayload.Value, 1337 );

    EXPECT_TRUE( Mailbox.IsEmpty() );
    EXPECT_FALSE( Mailbox.Pop().has_value() );
}

TEST( ParallelMailbox, MultiProducerDeliversAllMessagesExactlyOnce )
{
    LumenEngine::FMailBox Mailbox;

    constexpr LumenEngine::Int32 ProducerCount       = 4;
    constexpr LumenEngine::Int32 MessagesPerProducer = 500;
    constexpr LumenEngine::Int32 TotalMessages       = ProducerCount * MessagesPerProducer;

    std::barrier StartBarrier( ProducerCount + 1 );
    LumenEngine::TVector<std::jthread> Producers;
    Producers.reserve( ProducerCount );

    for ( LumenEngine::Int32 Producer = 0; Producer < ProducerCount; ++Producer )
    {
        Producers.emplace_back(
            [Producer, &Mailbox, &StartBarrier] ()
            {
                StartBarrier.arrive_and_wait();

                for ( LumenEngine::Int32 Index = 0; Index < MessagesPerProducer; ++Index )
                {
                    const LumenEngine::Int32 UniquePayload = ( Producer * 100000 ) + Index;
                    FValuePayload Payload{ UniquePayload };

                    Mailbox.Push( LumenEngine::FMessage::Make( 1U, static_cast<LumenEngine::ActorID>( Producer ), Payload ) );
                }
            } );
    }

    StartBarrier.arrive_and_wait();

    std::unordered_set<LumenEngine::Int32> SeenPayloads;
    SeenPayloads.reserve( TotalMessages );

    while ( static_cast<LumenEngine::Int32>( SeenPayloads.size() ) < TotalMessages )
    {
        LumenEngine::TOptional<LumenEngine::FMessage> Popped = Mailbox.Pop();
        if ( not Popped.has_value() )
        {
            std::this_thread::yield();
            continue;
        }

        const FValuePayload &PoppedPayload = Popped->GetPayload<FValuePayload>();
        SeenPayloads.insert( PoppedPayload.Value );
    }

    EXPECT_EQ( static_cast<LumenEngine::Int32>( SeenPayloads.size() ), TotalMessages );
    EXPECT_TRUE( Mailbox.IsEmpty() );
}

TEST( ParallelMailbox, FreeListReuseStress )
{
    LumenEngine::FMailBox Mailbox;
    constexpr LumenEngine::Int32 Iterations = 10000;

    /**
     * INFO: By pushing then popping in a loop, we force the FMailBox to
     * repeatedly reuse the same nodes from the FreeList.
     * This ensures that DeallocateNode followed by AllocateNode works
     * correctly for a single consumer.
     */
    for ( LumenEngine::Int32 i = 0; i < Iterations; ++i )
    {
        const LumenEngine::UInt32 UniqueType = static_cast<LumenEngine::UInt32>( i );
        Mailbox.Push( LumenEngine::FMessage::Make( UniqueType, 0ULL ) );

        LumenEngine::TOptional<LumenEngine::FMessage> Popped = Mailbox.Pop();
        ASSERT_TRUE( Popped.has_value() );
        EXPECT_EQ( Popped->Type, UniqueType );
    }

    EXPECT_TRUE( Mailbox.IsEmpty() );
}

TEST( ParallelMailbox, HighConcurrencyFIFOPerProducer )
{
    LumenEngine::FMailBox Mailbox;

    constexpr LumenEngine::Int32 ProducerCount       = 8;
    constexpr LumenEngine::Int32 MessagesPerProducer = 2000;
    constexpr LumenEngine::Int32 TotalMessages       = ProducerCount * MessagesPerProducer;

    std::barrier StartBarrier( ProducerCount + 1 );
    LumenEngine::TVector<std::jthread> Producers;
    Producers.reserve( ProducerCount );

    for ( LumenEngine::Int32 p = 0; p < ProducerCount; ++p )
    {
        Producers.emplace_back(
            [p, &Mailbox, &StartBarrier] ()
            {
                StartBarrier.arrive_and_wait();

                for ( LumenEngine::Int32 i = 0; i < MessagesPerProducer; ++i )
                {
                    // Type: Sequence number, Sender: Producer ID
                    Mailbox.Push( LumenEngine::FMessage::Make( static_cast<LumenEngine::UInt32>( i ), static_cast<LumenEngine::ActorID>( p ) ) );
                }
            } );
    }

    StartBarrier.arrive_and_wait();

    LumenEngine::Int32 ReceivedCount = 0;
    LumenEngine::TVector<LumenEngine::Int32> LastValueFromProducer( ProducerCount, -1 );

    /**
     * INFO: Single consumer pops all messages.
     * We check that for each producer, messages arrive in the correct sequence.
     * This ensures that Vyukov's FIFO properties are maintained and no
     * messages are lost or duplicated during concurrent Push operations.
     */
    while ( ReceivedCount < TotalMessages )
    {
        LumenEngine::TOptional<LumenEngine::FMessage> Msg = Mailbox.Pop();
        if ( Msg.has_value() )
        {
            ReceivedCount++;
            const LumenEngine::Int32 p   = static_cast<LumenEngine::Int32>( Msg->Sender );
            const LumenEngine::Int32 val = static_cast<LumenEngine::Int32>( Msg->Type );

            EXPECT_EQ( val, LastValueFromProducer[p] + 1 );
            LastValueFromProducer[p] = val;
        }
        else
        {
            // Transient gap or empty, yield to producers
            std::this_thread::yield();
        }
    }

    EXPECT_EQ( ReceivedCount, TotalMessages );
    EXPECT_TRUE( Mailbox.IsEmpty() );
}

TEST( ParallelMailbox, FreeListConcurrencyStress )
{
    LumenEngine::FMailBox Mailbox;

    constexpr LumenEngine::Int32 ProducerCount       = 12;
    constexpr LumenEngine::Int32 MessagesPerProducer = 1000;
    constexpr LumenEngine::Int32 TotalMessages       = ProducerCount * MessagesPerProducer;

    std::barrier StartBarrier( ProducerCount + 1 );

    // Producers push simultaneously
    LumenEngine::TVector<std::jthread> Producers;
    for ( LumenEngine::Int32 p = 0; p < ProducerCount; ++p )
    {
        Producers.emplace_back(
            [&Mailbox, &StartBarrier] ()
            {
                StartBarrier.arrive_and_wait();
                for ( LumenEngine::Int32 i = 0; i < MessagesPerProducer; ++i )
                {
                    Mailbox.Push( LumenEngine::FMessage::Make( static_cast<LumenEngine::UInt32>( i ), 0ULL ) );
                }
            } );
    }

    StartBarrier.arrive_and_wait();

    // Consumer pops simultaneously
    LumenEngine::Int32 TotalReceived = 0;
    while ( TotalReceived < TotalMessages )
    {
        LumenEngine::TOptional<LumenEngine::FMessage> Msg = Mailbox.Pop();
        if ( Msg )
        {
            TotalReceived++;
        }
        else
        {
            std::this_thread::yield();
        }
    }

    EXPECT_EQ( TotalReceived, TotalMessages );
    EXPECT_TRUE( Mailbox.IsEmpty() );
}

TEST( ParallelMailbox, IntenseFreeListChurnWithPayloadIntegrity )
{
    LumenEngine::FMailBox Mailbox;

    constexpr LumenEngine::Int32 ProducerCount       = 16;
    constexpr LumenEngine::Int32 MessagesPerProducer = 5000;
    constexpr LumenEngine::Int32 TotalMessages       = ProducerCount * MessagesPerProducer;

    std::barrier StartBarrier( ProducerCount + 1 );
    LumenEngine::TVector<std::jthread> Producers;

    for ( LumenEngine::Int32 p = 0; p < ProducerCount; ++p )
    {
        Producers.emplace_back(
            [p, &Mailbox, &StartBarrier] ()
            {
                StartBarrier.arrive_and_wait();
                for ( LumenEngine::Int32 i = 0; i < MessagesPerProducer; ++i )
                {
                    FValuePayload Payload;
                    Payload.Value = ( p * 1000000 ) + i;
                    Mailbox.Push( LumenEngine::FMessage::Make( 1U, static_cast<LumenEngine::ActorID>( p ), Payload ) );
                }
            } );
    }

    StartBarrier.arrive_and_wait();

    LumenEngine::Int32 ReceivedCount = 0;
    LumenEngine::TVector<LumenEngine::Int32> LastValuePerProducer( ProducerCount, -1 );

    /**
     * INFO: This test ensures that even with heavy node reuse (high churn),
     * no messages are lost, duplicated, or corrupted.
     * If the ABA problem in FreeList occurs, we expect this test to fail
     * with incorrect sequence numbers or a dead loop.
     */
    while ( ReceivedCount < TotalMessages )
    {
        LumenEngine::TOptional<LumenEngine::FMessage> Msg = Mailbox.Pop();
        if ( Msg )
        {
            const LumenEngine::Int32 p   = static_cast<LumenEngine::Int32>( Msg->Sender );
            const LumenEngine::Int32 val = Msg->GetPayload<FValuePayload>().Value;

            const LumenEngine::Int32 ProducerIdx = p;
            const LumenEngine::Int32 Sequence    = val % 1000000;

            EXPECT_EQ( Sequence, LastValuePerProducer[ProducerIdx] + 1 );
            LastValuePerProducer[ProducerIdx] = Sequence;
            ReceivedCount++;
        }
        else
        {
            std::this_thread::yield();
        }
    }

    EXPECT_EQ( ReceivedCount, TotalMessages );
    EXPECT_TRUE( Mailbox.IsEmpty() );
}

TEST( ParallelMailbox, MassiveNodeChurnStress )
{
    LumenEngine::FMailBox Mailbox;

    constexpr LumenEngine::Int32 ProducerCount = 8;
    constexpr LumenEngine::Int32 Iterations    = 20000;
    constexpr LumenEngine::Int32 TotalExpected = ProducerCount * Iterations;

    std::barrier StartBarrier( ProducerCount + 1 );
    LumenEngine::TVector<std::jthread> Producers;

    for ( LumenEngine::Int32 p = 0; p < ProducerCount; ++p )
    {
        Producers.emplace_back(
            [&Mailbox, &StartBarrier] ()
            {
                StartBarrier.arrive_and_wait();
                for ( LumenEngine::Int32 i = 0; i < Iterations; ++i )
                {
                    Mailbox.Push( LumenEngine::FMessage::Make( 1U, 0ULL ) );
                }
            } );
    }

    StartBarrier.arrive_and_wait();

    LumenEngine::Int32 Received = 0;
    while ( Received < TotalExpected )
    {
        if ( Mailbox.Pop() )
        {
            Received++;
        }
    }

    EXPECT_EQ( Received, TotalExpected );
}