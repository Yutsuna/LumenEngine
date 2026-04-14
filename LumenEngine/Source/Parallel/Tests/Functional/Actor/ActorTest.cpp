#include "Actor/Actor.hpp"
#include "Container/Vector.hpp"
#include "CoreTypes.hpp"

#include <barrier>
#include <gtest/gtest.h>
#include <thread>

namespace
{

class FCollectingActor final : public LumenEngine::AActor
{
public:

    explicit FCollectingActor ( LumenEngine::ActorID InId ) noexcept : LumenEngine::AActor( InId )
    {
        /* Ctor */
    }

    void Receive ( LumenEngine::FMessage InMessage ) override
    {
        ReceivedMessages.push_back( InMessage );
    }

    LumenEngine::TVector<LumenEngine::FMessage> ReceivedMessages;
};

struct FTestPayload
{
    LumenEngine::Int32 Value = 0;
};

} // namespace

TEST( ParallelActor, ProcessMailboxDrainsAllEnqueuedMessages )
{
    FCollectingActor Actor( 1ULL );

    Actor.EnqueueMessage( LumenEngine::FMessage::Make( 1U, 10ULL, FTestPayload{ 100 } ) );
    Actor.EnqueueMessage( LumenEngine::FMessage::Make( 2U, 20ULL, FTestPayload{ 200 } ) );
    Actor.EnqueueMessage( LumenEngine::FMessage::Make( 3U, 30ULL, FTestPayload{ 300 } ) );

    Actor.ProcessMailbox();

    ASSERT_EQ( Actor.ReceivedMessages.size(), 3U );

    EXPECT_EQ( Actor.ReceivedMessages[0].Type, 1U );
    EXPECT_EQ( Actor.ReceivedMessages[1].Type, 2U );
    EXPECT_EQ( Actor.ReceivedMessages[2].Type, 3U );

    EXPECT_EQ( Actor.ReceivedMessages[0].GetPayload<FTestPayload>().Value, 100 );
    EXPECT_EQ( Actor.ReceivedMessages[1].GetPayload<FTestPayload>().Value, 200 );
    EXPECT_EQ( Actor.ReceivedMessages[2].GetPayload<FTestPayload>().Value, 300 );
}

TEST( ParallelActor, AcceptsConcurrentProducers )
{
    FCollectingActor Actor( 3ULL );

    constexpr LumenEngine::Int32 ProducerCount       = 4;
    constexpr LumenEngine::Int32 MessagesPerProducer = 300;

    std::barrier StartBarrier( ProducerCount + 1 );
    LumenEngine::TVector<std::jthread> Producers;
    Producers.reserve( ProducerCount );

    for ( LumenEngine::Int32 Producer = 0; Producer < ProducerCount; ++Producer )
    {
        Producers.emplace_back(
            [Producer, &Actor, &StartBarrier] ()
            {
                StartBarrier.arrive_and_wait();

                for ( LumenEngine::Int32 Index = 0; Index < MessagesPerProducer; ++Index )
                {
                    FTestPayload Payload{ ( Producer * 100000 ) + Index };
                    Actor.EnqueueMessage(
                        LumenEngine::FMessage::Make( static_cast<LumenEngine::UInt32>( Producer ), static_cast<LumenEngine::ActorID>( Producer ), Payload ) );
                }
            } );
    }

    StartBarrier.arrive_and_wait();
    Producers.clear();
    Actor.ProcessMailbox();

    const LumenEngine::USize ExpectedCount = static_cast<LumenEngine::USize>( ProducerCount ) * static_cast<LumenEngine::USize>( MessagesPerProducer );
    EXPECT_EQ( Actor.ReceivedMessages.size(), ExpectedCount );
}