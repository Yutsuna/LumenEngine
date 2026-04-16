/**
 * @file ActorTest.cpp
 * @brief Functional tests for AActor base class logic.
 */

#include "Actor/Actor.hpp"
#include <gtest/gtest.h>

namespace
{

class FFunctionalActor final : public LumenEngine::AActor
{
public:

    explicit FFunctionalActor ( LumenEngine::ActorID InId ) noexcept : AActor( InId )
    {
        /* Ctor*/
    }

    void Receive ( const LumenEngine::FMessage &InMessage ) override
    {
        LastType = InMessage.Type;
        ReceivedCount++;
    }

    LumenEngine::UInt32 LastType      = 0U;
    LumenEngine::UInt32 ReceivedCount = 0U;
};

} // namespace

TEST( ParallelActor, ActorLifecycle )
{
    using namespace LumenEngine;

    FFunctionalActor Actor( 500ULL );

    EXPECT_EQ( Actor.GetId(), 500ULL );
    EXPECT_EQ( Actor.ReceivedCount, 0U );

    /** Test mailbox processing loop */
    Actor.EnqueueMessage( FMessage::Make( 10U, 1ULL ) );
    Actor.EnqueueMessage( FMessage::Make( 20U, 1ULL ) );
    Actor.EnqueueMessage( FMessage::Make( 30U, 1ULL ) );

    /** Branch: ProcessMailbox draining multiple messages */
    Actor.ProcessMailbox();

    EXPECT_EQ( Actor.ReceivedCount, 3U );
    EXPECT_EQ( Actor.LastType, 30U );

    /** Branch: ProcessMailbox on empty */
    Actor.ProcessMailbox();
    EXPECT_EQ( Actor.ReceivedCount, 3U );
}

TEST( ParallelActor, ActorReferenceStability )
{
    using namespace LumenEngine;

    FFunctionalActor Actor( 1ULL );
    FActorRef Ref = Actor.GetRef();

    EXPECT_EQ( Ref.GetId(), Actor.GetId() );

    FMessage Msg = FMessage::Make( 99U, 1ULL );
    Ref.EnqueueMessage( Msg );

    Actor.ProcessMailbox();
    EXPECT_EQ( Actor.LastType, 99U );
}