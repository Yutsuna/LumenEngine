/**
 * @file ActorMailboxPushTests.cpp
 * @brief Tests that the mailbox push operation returns true on success.
 */

#include <Actor/ActorMailbox.hpp>
#include <Actor/ActorMessage.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( FMailBox_Push, ReturnsTrueOnSuccess )
{
    FMailBox Box;
    const Bool Result = Box.Push( FMessage::Make( 1U, 0ULL ) );
    LUMEN_CHECK_TRUE( Result );
}

LUMEN_TEST( FMailBox_Push, ReturnsTrueAcrossManyPushes )
{
    FMailBox Box;
    for ( UInt32 Index = 0U; Index < 256U; ++Index )
    {
        LUMEN_CHECK_TRUE( Box.Push( FMessage::Make( Index, 0ULL ) ) );
    }
}

} // namespace Lumen
