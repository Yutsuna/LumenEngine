/**
 * @file ActorTypesTest.cpp
 * @brief Functional tests for FActorRef and Actor IDs.
 */

#include "Actor/ActorTypes.hpp"
#include "Actor/Actor.hpp"
#include <gtest/gtest.h>

namespace
{
class FActorRefMock final : public LumenEngine::AActor
{
public:

    explicit FActorRefMock ( LumenEngine::ActorID InId ) noexcept : AActor( InId )
    {
        /* Ctor*/
    }

    void Receive ( const LumenEngine::FMessage & /*InMessage*/ ) override
    {
        bReceived = true;
    }
    LumenEngine::Bool bReceived = false;
};
} // namespace

TEST( ParallelActor, ActorRefBehavior )
{
    using namespace LumenEngine;

    /** Branch: Invalid / Default Ref */
    FActorRef InvalidRef;
    EXPECT_FALSE( InvalidRef.IsValid() );
    EXPECT_EQ( InvalidRef.GetId(), 0ULL );

    /** Branch: Valid Ref from Actor */
    FActorRefMock MockActor( 10ULL );
    FActorRef ValidRef = MockActor.GetRef();

    EXPECT_TRUE( ValidRef.IsValid() );
    EXPECT_EQ( ValidRef.GetId(), 10ULL );

    /** Branch: EnqueueMessage through valid ref */
    ValidRef.EnqueueMessage( FMessage::Make( 1U, 0ULL ) );
    MockActor.ProcessMailbox();
    EXPECT_TRUE( MockActor.bReceived );

    /** Branch: EnqueueMessage through null ActorPtr (Safety check) */
    FActorRef NullPtrRef( 20ULL, nullptr );
    NullPtrRef.EnqueueMessage( FMessage::Make( 2U, 0ULL ) );
}

TEST( ParallelActor, ActorRefComparison )
{
    using namespace LumenEngine;

    FActorRef RefA( 1ULL, nullptr );
    FActorRef RefB( 2ULL, nullptr );
    FActorRef RefAAgain( 1ULL, nullptr );

    EXPECT_TRUE( RefA == RefAAgain );
    EXPECT_TRUE( RefA != RefB );
    EXPECT_LT( RefA, RefB );
}