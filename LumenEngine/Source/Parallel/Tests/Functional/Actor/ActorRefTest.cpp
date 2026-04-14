/**
 * @file ActorRefTest.cpp
 * @brief Unit tests for FActorRef in Lumen Engine.
 */

#include "Actor/Actor.hpp"
#include "Actor/ActorMessage.hpp"
#include "Actor/ActorTypes.hpp"

#include <gtest/gtest.h>

namespace LumenEngine
{

/**
 * @class FTestActor
 * @brief Simple actor for testing FActorRef and FMailBox interaction.
 */
class FTestActor final : public AActor
{
public:

    explicit FTestActor ( ActorID InId ) noexcept : AActor( InId )
    {
    }

    void Receive ( const FMessage &InMessage ) override
    {
        LastReceivedType = InMessage.Type;
        LastSender       = InMessage.Sender;
        ReceivedCount++;
    }

    UInt32 LastReceivedType = 0;
    ActorID LastSender      = 0ULL;
    Int32 ReceivedCount     = 0;
};

} // namespace LumenEngine

TEST( ParallelActorRef, BasicFunctionality )
{
    using namespace LumenEngine;

    FTestActor Actor( 42ULL );
    FActorRef Ref = Actor.GetRef();

    EXPECT_TRUE( Ref.IsValid() );
    EXPECT_EQ( Ref.GetId(), 42ULL );
}

TEST( ParallelActorRef, SendMessageThroughRef )
{
    using namespace LumenEngine;

    FTestActor Actor( 101ULL );
    FActorRef Ref = Actor.GetRef();

    const FMessage Msg = FMessage::Make( 1337U, 7ULL );
    Ref.EnqueueMessage( Msg );

    // Message should be in the mailbox but not processed yet
    EXPECT_EQ( Actor.ReceivedCount, 0 );

    Actor.ProcessMailbox();

    EXPECT_EQ( Actor.ReceivedCount, 1 );
    EXPECT_EQ( Actor.LastReceivedType, 1337U );
    EXPECT_EQ( Actor.LastSender, 7ULL );
}

TEST( ParallelActorRef, InvalidRefSafety )
{
    using namespace LumenEngine;

    FActorRef InvalidRef;
    EXPECT_FALSE( InvalidRef.IsValid() );
    EXPECT_EQ( InvalidRef.GetId(), 0ULL );

    const FMessage Msg = FMessage::Make( 1U, 0ULL );

    // Should not crash when sending to invalid ref (it should just do nothing)
    EXPECT_NO_THROW( InvalidRef.EnqueueMessage( Msg ) );
}

TEST( ParallelActorRef, ComparisonOperators )
{
    using namespace LumenEngine;

    FTestActor ActorA( 1ULL );
    FTestActor ActorB( 2ULL );

    FActorRef RefA1 = ActorA.GetRef();
    FActorRef RefA2 = ActorA.GetRef();
    FActorRef RefB  = ActorB.GetRef();

    EXPECT_EQ( RefA1, RefA2 );
    EXPECT_NE( RefA1, RefB );
    EXPECT_LT( RefA1, RefB );
}
