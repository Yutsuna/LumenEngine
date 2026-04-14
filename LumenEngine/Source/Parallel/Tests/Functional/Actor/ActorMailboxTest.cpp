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