/**
 * @file ActorOptimizationsTest.cpp
 * @brief Functional tests for Actor sorting and Mailbox throttling.
 */

#include "Actor/Actor.hpp"
#include "Container/SharedPtr.hpp"
#include "Container/Vector.hpp"
#include <algorithm>
#include <gtest/gtest.h>

namespace
{

class FTestActor final : public LumenEngine::AActor
{
    LUMEN_ACTOR_BODY ( FTestActor );

public:

    FTestActor ( LumenEngine::ActorID InId ) noexcept : AActor( InId )
    {
        /* Ctor */
    }

    void Receive ( const LumenEngine::FMessage & ) override
    {
        ++ReceivedCount;
    }

    LumenEngine::UInt32 ReceivedCount = 0U;
};

class FAnotherActor final : public LumenEngine::AActor
{
    LUMEN_ACTOR_BODY ( FAnotherActor );

public:

    FAnotherActor ( LumenEngine::ActorID InId ) noexcept : AActor( InId )
    {
        /* Ctor */
    }

    void Receive ( const LumenEngine::FMessage & ) override
    {
    }
};

} // namespace

TEST( ParallelActor, MailboxThrottling )
{
    using namespace LumenEngine;

    FTestActor Actor( 1ULL );

    const UInt32 TotalMessages = 50U;
    const UInt32 ThrottleLimit = 20U;

    for ( UInt32 Index = 0; Index < TotalMessages; ++Index )
    {
        Actor.EnqueueMessage( FMessage::Make( Index, 0ULL ) );
    }

    /** Test throttling */
    Actor.ProcessMailbox( ThrottleLimit );
    EXPECT_EQ( Actor.ReceivedCount, ThrottleLimit );

    /** Process remaining */
    Actor.ProcessMailbox( 0U );
    EXPECT_EQ( Actor.ReceivedCount, TotalMessages );
}

TEST( ParallelActor, TypeIndexSorting )
{
    using namespace LumenEngine;

    TVector<TSharedPtr<AActor>> Actors;
    Actors.emplace_back( MakeShared<FAnotherActor>( 1ULL ) );
    Actors.emplace_back( MakeShared<FTestActor>( 2ULL ) );
    Actors.emplace_back( MakeShared<FAnotherActor>( 3ULL ) );
    Actors.emplace_back( MakeShared<FTestActor>( 4ULL ) );

    /** Automatic sorting by type index */
    std::ranges::sort( Actors, [] ( const TSharedPtr<AActor> &A, const TSharedPtr<AActor> &B ) { return A->GetTypeID() < B->GetTypeID(); } );

    /** Verify they are grouped by type */
    EXPECT_EQ( Actors[0]->GetTypeID(), Actors[1]->GetTypeID() );
    EXPECT_EQ( Actors[2]->GetTypeID(), Actors[3]->GetTypeID() );
    EXPECT_NE( Actors[0]->GetTypeID(), Actors[2]->GetTypeID() );
}
