/**
 * @file ActorMailboxFenceTest.cpp
 * @brief Tests the fence mechanism in the actor mailbox.
 */

#include <Actor/ActorMailbox.hpp>
#include <Actor/ActorMessage.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( FMailBox_Fence, PreFenceMessagesAreVisible )
{
    FMailBox Box;
    Box.Push( FMessage::Make( 1U, 0ULL ) );
    Box.BeginTick();

    LUMEN_CHECK_TRUE( Box.Pop().HasValue() );
    LUMEN_CHECK_FALSE( Box.Pop().HasValue() );

    Box.EndTick();
}

LUMEN_TEST( FMailBox_Fence, PostFenceMessagesWaitNextTick )
{
    FMailBox Box;
    Box.BeginTick();

    Box.Push( FMessage::Make( 1U, 0ULL ) );
    LUMEN_CHECK_FALSE( Box.Pop().HasValue() );

    Box.BeginTick();
    LUMEN_CHECK_TRUE( Box.Pop().HasValue() );

    Box.EndTick();
}

LUMEN_TEST( FMailBox_Fence, EndTickRestoresUnboundedProcessing )
{
    FMailBox Box;
    Box.BeginTick();
    Box.Push( FMessage::Make( 1U, 0ULL ) );
    Box.EndTick();

    LUMEN_CHECK_TRUE( Box.Pop().HasValue() );
}

} // namespace Lumen
