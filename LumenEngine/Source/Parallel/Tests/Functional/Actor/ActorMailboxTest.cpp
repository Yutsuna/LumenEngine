#include "Actor/ActorMailbox.hpp"

#include <gtest/gtest.h>

#include <any>
#include <barrier>
#include <thread>
#include <unordered_set>
#include <vector>

TEST( ParallelMailbox, EmptyInitially )
{
    LumenEngine::FMailBox Mailbox;

    EXPECT_TRUE( Mailbox.IsEmpty() );
    EXPECT_FALSE( Mailbox.Pop().has_value() );
}

TEST( ParallelMailbox, PushThenPopSingleMessage )
{
    LumenEngine::FMailBox Mailbox;

    Mailbox.Push( LumenEngine::FMessage::Make( 42U, 7ULL, 1337 ) );

    ASSERT_FALSE( Mailbox.IsEmpty() );

    LumenEngine::TOptional<LumenEngine::FMessage> Popped = Mailbox.Pop();
    ASSERT_TRUE( Popped.has_value() );
    EXPECT_EQ( Popped->Type, 42U );
    EXPECT_EQ( Popped->Sender, 7ULL );
    ASSERT_TRUE( Popped->Payload.has_value() );
    EXPECT_EQ( std::any_cast<int>( Popped->Payload ), 1337 );

    EXPECT_TRUE( Mailbox.IsEmpty() );
    EXPECT_FALSE( Mailbox.Pop().has_value() );
}

TEST( ParallelMailbox, PreservesFifoOrderSingleProducer )
{
    LumenEngine::FMailBox Mailbox;

    constexpr int MessageCount = 256;
    for ( int Index = 0; Index < MessageCount; ++Index )
    {
        Mailbox.Push( LumenEngine::FMessage::Make( static_cast<LumenEngine::UInt32>( Index ), 1ULL, Index ) );
    }

    for ( int Index = 0; Index < MessageCount; ++Index )
    {
        LumenEngine::TOptional<LumenEngine::FMessage> Popped = Mailbox.Pop();
        ASSERT_TRUE( Popped.has_value() );
        EXPECT_EQ( Popped->Type, static_cast<LumenEngine::UInt32>( Index ) );
        EXPECT_EQ( Popped->Sender, 1ULL );
        ASSERT_TRUE( Popped->Payload.has_value() );
        EXPECT_EQ( std::any_cast<int>( Popped->Payload ), Index );
    }

    EXPECT_TRUE( Mailbox.IsEmpty() );
}

TEST( ParallelMailbox, MultiProducerDeliversAllMessagesExactlyOnce )
{
    LumenEngine::FMailBox Mailbox;

    constexpr int ProducerCount       = 4;
    constexpr int MessagesPerProducer = 500;
    constexpr int TotalMessages       = ProducerCount * MessagesPerProducer;

    std::barrier StartBarrier( ProducerCount + 1 );
    std::vector<std::jthread> Producers;
    Producers.reserve( ProducerCount );

    for ( int Producer = 0; Producer < ProducerCount; ++Producer )
    {
        Producers.emplace_back(
            [Producer, &Mailbox, &StartBarrier] ()
            {
                StartBarrier.arrive_and_wait();

                for ( int Index = 0; Index < MessagesPerProducer; ++Index )
                {
                    const int UniquePayload = ( Producer * 100000 ) + Index;
                    Mailbox.Push( LumenEngine::FMessage::Make( 1U, static_cast<LumenEngine::ActorID>( Producer ), UniquePayload ) );
                }
            } );
    }

    StartBarrier.arrive_and_wait();

    std::unordered_set<int> SeenPayloads;
    SeenPayloads.reserve( TotalMessages );

    while ( static_cast<int>( SeenPayloads.size() ) < TotalMessages )
    {
        LumenEngine::TOptional<LumenEngine::FMessage> Popped = Mailbox.Pop();
        if ( not Popped.has_value() )
        {
            std::this_thread::yield();
            continue;
        }

        ASSERT_TRUE( Popped->Payload.has_value() );
        SeenPayloads.insert( std::any_cast<int>( Popped->Payload ) );
    }

    EXPECT_EQ( static_cast<int>( SeenPayloads.size() ), TotalMessages );

    for ( int Producer = 0; Producer < ProducerCount; ++Producer )
    {
        for ( int Index = 0; Index < MessagesPerProducer; ++Index )
        {
            const int UniquePayload = ( Producer * 100000 ) + Index;
            EXPECT_TRUE( SeenPayloads.contains( UniquePayload ) );
        }
    }

    EXPECT_TRUE( Mailbox.IsEmpty() );
}

TEST( ParallelMailbox, RepeatedPopOnEmptyAlwaysReturnsNoValue )
{
    LumenEngine::FMailBox Mailbox;

    for ( int Attempt = 0; Attempt < 16; ++Attempt )
    {
        EXPECT_FALSE( Mailbox.Pop().has_value() );
        EXPECT_TRUE( Mailbox.IsEmpty() );
    }
}

TEST( ParallelMailbox, MessageFactoryPopulatesAllFields )
{
    LumenEngine::FMailBox Mailbox;

    Mailbox.Push( LumenEngine::FMessage::Make( 77U, 1234ULL, std::string( "Payload" ) ) );

    LumenEngine::TOptional<LumenEngine::FMessage> Popped = Mailbox.Pop();
    ASSERT_TRUE( Popped.has_value() );
    EXPECT_EQ( Popped->Type, 77U );
    EXPECT_EQ( Popped->Sender, 1234ULL );
    EXPECT_EQ( std::any_cast<std::string>( Popped->Payload ), "Payload" );
}
