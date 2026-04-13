#include "Actor/Actor.hpp"

#include <gtest/gtest.h>

#include <any>
#include <barrier>
#include <string>
#include <thread>
#include <vector>

namespace
{

class FCollectingActor final : public LumenEngine::AActor
{
public:

    explicit FCollectingActor ( LumenEngine::ActorID InId ) : LumenEngine::AActor( InId )
    {
    }

    void Receive ( LumenEngine::FMessage InMessage ) override
    {
        ReceivedMessages.push_back( std::move( InMessage ) );
    }

    std::vector<LumenEngine::FMessage> ReceivedMessages;
};

} // namespace

TEST( ParallelActor, ExposesConfiguredId )
{
    FCollectingActor Actor( 99ULL );
    EXPECT_EQ( Actor.GetId(), 99ULL );
}

TEST( ParallelActor, ProcessMailboxDrainsAllEnqueuedMessages )
{
    FCollectingActor Actor( 1ULL );

    Actor.EnqueueMessage( LumenEngine::FMessage::Make( 1U, 10ULL, 100 ) );
    Actor.EnqueueMessage( LumenEngine::FMessage::Make( 2U, 20ULL, 200 ) );
    Actor.EnqueueMessage( LumenEngine::FMessage::Make( 3U, 30ULL, 300 ) );

    Actor.ProcessMailbox();

    ASSERT_EQ( Actor.ReceivedMessages.size(), 3U );

    EXPECT_EQ( Actor.ReceivedMessages[0].Type, 1U );
    EXPECT_EQ( Actor.ReceivedMessages[1].Type, 2U );
    EXPECT_EQ( Actor.ReceivedMessages[2].Type, 3U );

    EXPECT_EQ( std::any_cast<int>( Actor.ReceivedMessages[0].Payload ), 100 );
    EXPECT_EQ( std::any_cast<int>( Actor.ReceivedMessages[1].Payload ), 200 );
    EXPECT_EQ( std::any_cast<int>( Actor.ReceivedMessages[2].Payload ), 300 );

    Actor.ProcessMailbox();
    EXPECT_EQ( Actor.ReceivedMessages.size(), 3U );
}

TEST( ParallelActor, ProcessMailboxOnEmptyDoesNothing )
{
    FCollectingActor Actor( 2ULL );

    Actor.ProcessMailbox();

    EXPECT_TRUE( Actor.ReceivedMessages.empty() );
}

TEST( ParallelActor, AcceptsConcurrentProducers )
{
    FCollectingActor Actor( 3ULL );

    constexpr int ProducerCount       = 4;
    constexpr int MessagesPerProducer = 300;

    std::barrier StartBarrier( ProducerCount + 1 );
    std::vector<std::jthread> Producers;
    Producers.reserve( ProducerCount );

    for ( int Producer = 0; Producer < ProducerCount; ++Producer )
    {
        Producers.emplace_back(
            [Producer, &Actor, &StartBarrier] ()
            {
                StartBarrier.arrive_and_wait();

                for ( int Index = 0; Index < MessagesPerProducer; ++Index )
                {
                    Actor.EnqueueMessage( LumenEngine::FMessage::Make( static_cast<LumenEngine::UInt32>( Producer ), static_cast<LumenEngine::ActorID>( Producer ),
                                                                       ( Producer * 100000 ) + Index ) );
                }
            } );
    }

    StartBarrier.arrive_and_wait();

    Producers.clear();

    Actor.ProcessMailbox();

    const std::size_t ExpectedCount = static_cast<std::size_t>( ProducerCount ) * static_cast<std::size_t>( MessagesPerProducer );
    EXPECT_EQ( Actor.ReceivedMessages.size(), ExpectedCount );
}

TEST( ParallelActor, CanProcessMailboxIncrementallyUntilAllMessagesArrive )
{
    FCollectingActor Actor( 4ULL );

    constexpr int ProducerCount       = 3;
    constexpr int MessagesPerProducer = 200;
    constexpr int TotalMessages       = ProducerCount * MessagesPerProducer;

    std::barrier StartBarrier( ProducerCount + 1 );
    std::vector<std::jthread> Producers;
    Producers.reserve( ProducerCount );

    for ( int Producer = 0; Producer < ProducerCount; ++Producer )
    {
        Producers.emplace_back(
            [Producer, &Actor, &StartBarrier] ()
            {
                StartBarrier.arrive_and_wait();

                for ( int Index = 0; Index < MessagesPerProducer; ++Index )
                {
                    Actor.EnqueueMessage( LumenEngine::FMessage::Make( 9U, static_cast<LumenEngine::ActorID>( Producer ), ( Producer * 100000 ) + Index ) );
                }
            } );
    }

    StartBarrier.arrive_and_wait();

    while ( static_cast<int>( Actor.ReceivedMessages.size() ) < TotalMessages )
    {
        Actor.ProcessMailbox();
        std::this_thread::yield();
    }

    Producers.clear();
    Actor.ProcessMailbox();

    EXPECT_EQ( static_cast<int>( Actor.ReceivedMessages.size() ), TotalMessages );
}

TEST( ParallelActor, PreservesPayloadTypeAcrossMailbox )
{
    FCollectingActor Actor( 5ULL );

    Actor.EnqueueMessage( LumenEngine::FMessage::Make( 11U, 99ULL, std::string( "Text" ) ) );
    Actor.EnqueueMessage( LumenEngine::FMessage::Make( 12U, 88ULL, 2048 ) );

    Actor.ProcessMailbox();

    ASSERT_EQ( Actor.ReceivedMessages.size(), 2U );
    EXPECT_EQ( std::any_cast<std::string>( Actor.ReceivedMessages[0].Payload ), "Text" );
    EXPECT_EQ( std::any_cast<int>( Actor.ReceivedMessages[1].Payload ), 2048 );
}
