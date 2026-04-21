/**
 * @file ActorMailboxTest.cpp
 * @brief Functional tests for FMailBox to ensure 100% branch coverage.
 */

#include "Actor/ActorMailbox.hpp"
#include <gtest/gtest.h>

/**
 * @brief Test for the lock-free MPSC mailbox.
 */
TEST( ParallelActor, MailboxOperations )
{
    using namespace LumenEngine;

    FMailBox Mailbox;

    /** Branch: IsEmpty() on new mailbox */
    EXPECT_TRUE( Mailbox.IsEmpty() );

    /** Branch: Pop() on empty mailbox */
    EXPECT_FALSE( Mailbox.Pop().has_value() );

    /** Branch: Push() using new allocation (pool starts empty) */
    Mailbox.Push( FMessage::Make( 1U, 10ULL ) );
    EXPECT_FALSE( Mailbox.IsEmpty() );

    /** Branch: Pop() valid message and return node to pool */
    TOptional<FMessage> Msg1 = Mailbox.Pop();
    ASSERT_TRUE( Msg1.has_value() );
    EXPECT_EQ( Msg1->Type, 1U );
    EXPECT_TRUE( Mailbox.IsEmpty() );

    /** Branch: Push() using pooled node */
    Mailbox.Push( FMessage::Make( 2U, 20ULL ) );
    TOptional<FMessage> Msg2 = Mailbox.Pop();
    EXPECT_EQ( Msg2->Type, 2U );

    /**
     * Branch Coverage: Node Pooling & Overflow
     * The internal pool (FreeNodes) has a capacity of 256.
     */
    const UInt32 OverflowCount = 300U;

    // Push 300 messages to force 'new' allocations once pool (0) is empty
    for ( UInt32 Index = 0U; Index < OverflowCount; ++Index )
    {
        Mailbox.Push( FMessage::Make( Index, 100ULL ) );
    }

    /**
     * Pop 300 messages.
     * The first 256 nodes popped will return to the pool.
     * The remaining 44 will hit the 'delete' branch because the pool is full.
     */
    for ( UInt32 Index = 0U; Index < OverflowCount; ++Index )
    {
        TOptional<FMessage> Popped = Mailbox.Pop();
        ASSERT_TRUE( Popped.has_value() );
        EXPECT_EQ( Popped->Type, Index );
    }

    EXPECT_TRUE( Mailbox.IsEmpty() );
}

TEST( ParallelActor, MailboxReserve )
{
    using namespace LumenEngine;

    FMailBox Mailbox;

    /** Edge Case: Reserve zero should do nothing and not crash */
    Mailbox.Reserve( 0ULL );
    EXPECT_TRUE( Mailbox.IsEmpty() );

    /**
     * Pre-allocate 10 nodes.
     * We can't directly inspect the free list, but we can verify that
     * push operations succeed after reservation.
     */
    Mailbox.Reserve( 10ULL );

    for ( UInt32 Index = 0U; Index < 10U; ++Index )
    {
        Mailbox.Push( FMessage::Make( Index, 100ULL ) );
    }

    for ( UInt32 Index = 0U; Index < 10U; ++Index )
    {
        TOptional<FMessage> Popped = Mailbox.Pop();
        ASSERT_TRUE( Popped.has_value() );
        EXPECT_EQ( Popped->Type, Index );
    }

    EXPECT_TRUE( Mailbox.IsEmpty() );

    /**
     * Edge Case: Over-reserve (Pool capacity is 256)
     * This triggers the 'FreeNodes.Push' failure branch in Reserve().
     */
    Mailbox.Reserve( 1000ULL );

    /** Verify it still works after over-reservation */
    Mailbox.Push( FMessage::Make( 999U, 0ULL ) );
    TOptional<FMessage> Popped = Mailbox.Pop();
    ASSERT_TRUE( Popped.has_value() );
    EXPECT_EQ( Popped->Type, 999U );

    /**
     * Edge Case: Multiple reservations
     */
    Mailbox.Reserve( 5ULL );
    Mailbox.Reserve( 5ULL );

    for ( UInt32 Index = 0U; Index < 10U; ++Index )
    {
        Mailbox.Push( FMessage::Make( Index, 100ULL ) );
    }

    for ( UInt32 Index = 0U; Index < 10U; ++Index )
    {
        EXPECT_TRUE( Mailbox.Pop().has_value() );
    }

    EXPECT_TRUE( Mailbox.IsEmpty() );
}

TEST( ParallelActor, MailboxDestructorCleanup )
{
    using namespace LumenEngine;

    /** Branch: Destructor cleaning up unconsumed nodes and pool */
    {
        FMailBox Mailbox;
        Mailbox.Push( FMessage::Make( 1U, 1ULL ) );
        Mailbox.Push( FMessage::Make( 2U, 2ULL ) );
    }
    SUCCEED();
}