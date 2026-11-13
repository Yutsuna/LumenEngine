/*
 * @file ActorMailboxTests.cpp
 * @brief Functional tests for FMailBox.
 */

#include <Actor/ActorMailbox.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

#include <Templates/Optional.hpp>
#include <Thread/Thread.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Test message fixtures
 */

namespace
{

    /** @brief Minimal payload reused across push/pop round trips. */
    struct FBooleanPayload
    {
        Bool bFlag  = false;
        Int32 Value = 0;
    };

    constexpr UInt32 TestMessageType = 7777U;
    constexpr ActorID TestSenderId   = 31415ULL;

    [[nodiscard]] FMessage MakeTestMessage ( const UInt32 InIndex, const ActorID InSender, const Int32 InValue ) noexcept
    {
        return FMessage::Make( TestMessageType + InIndex, InSender, InValue );
    }

} // namespace

/**
 * initial state
 */

LUMEN_TEST( FMailBox_InitialState, StartsEmpty )
{
    FMailBox Mailbox;

    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

LUMEN_TEST( FMailBox_InitialState, PopOnEmptySkipsMessage )
{
    FMailBox Mailbox;

    const TOptional<FMessage> FirstPop  = Mailbox.Pop();
    const TOptional<FMessage> SecondPop = Mailbox.Pop();

    LUMEN_CHECK_FALSE( FirstPop.HasValue() );
    LUMEN_CHECK_FALSE( SecondPop.HasValue() );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

/**
 * Push / Pop round trips
 */

LUMEN_TEST( FMailBox_PushPop, SingleRoundTrip )
{
    FMailBox Mailbox;

    const FMessage Pushed = MakeTestMessage( 0U, TestSenderId, 1 );

    Mailbox.Push( Pushed );

    LUMEN_CHECK_FALSE( Mailbox.IsEmpty() );

    const TOptional<FMessage> Popped = Mailbox.Pop();

    LUMEN_CHECK_TRUE( Popped.HasValue() );
    LUMEN_CHECK_EQ( ( *Popped ).Sender, TestSenderId );
    LUMEN_CHECK_EQ( ( *Popped ).Type, TestMessageType );
    LUMEN_CHECK_EQ( ( *Popped ).GetPayload<Int32>(), 1 );

    /** INFO: The queue must be empty again once the message is consumed. */
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );

    const TOptional<FMessage> ExtraPop = Mailbox.Pop();

    LUMEN_CHECK_FALSE( ExtraPop.HasValue() );
}

LUMEN_TEST( FMailBox_PushPop, BatchPreservesFifoOrder )
{
    FMailBox Mailbox;

    for ( UInt32 Index = 0U; Index < 5U; ++Index )
    {
        Mailbox.Push( MakeTestMessage( Index, TestSenderId, static_cast<Int32>( Index ) ) );
    }

    for ( UInt32 ExpectedIndex = 0U; ExpectedIndex < 5U; ++ExpectedIndex )
    {
        const TOptional<FMessage> Popped = Mailbox.Pop();

        LUMEN_CHECK_TRUE( Popped.HasValue() );
        LUMEN_CHECK_EQ( ( *Popped ).Type, TestMessageType + ExpectedIndex );
        LUMEN_CHECK_EQ( ( *Popped ).GetPayload<Int32>(), static_cast<Int32>( ExpectedIndex ) );
    }

    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

/**
 * Queue gaps
 */

LUMEN_TEST( FMailBox_QueueGap, PopBetweenPushesRelinksStub )
{
    /**
     * INFO: Popping the last pending message relinks the consumed node as the
     *       new stub; the next push must still be deliverable.
     */
    FMailBox Mailbox;

    Mailbox.Push( MakeTestMessage( 0U, TestSenderId, 10 ) );

    const TOptional<FMessage> FirstPop = Mailbox.Pop();

    LUMEN_CHECK_TRUE( FirstPop.HasValue() );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );

    Mailbox.Push( MakeTestMessage( 1U, TestSenderId, 20 ) );

    const TOptional<FMessage> SecondPop = Mailbox.Pop();

    LUMEN_CHECK_TRUE( SecondPop.HasValue() );
    LUMEN_CHECK_EQ( ( *SecondPop ).Type, TestMessageType + 1U );
    LUMEN_CHECK_EQ( ( *SecondPop ).GetPayload<Int32>(), 20 );
}

LUMEN_TEST( FMailBox_QueueGap, RepeatedDrainCyclesStayConsistent )
{
    FMailBox Mailbox;

    for ( UInt32 Cycle = 0U; Cycle < 1000U; ++Cycle )
    {
        Mailbox.Push( MakeTestMessage( Cycle, TestSenderId, static_cast<Int32>( Cycle ) ) );

        const TOptional<FMessage> Popped = Mailbox.Pop();

        LUMEN_CHECK_TRUE( Popped.HasValue() );
        LUMEN_CHECK_EQ( ( *Popped ).Type, TestMessageType + Cycle );
        LUMEN_CHECK_EQ( ( *Popped ).GetPayload<Int32>(), static_cast<Int32>( Cycle ) );
        LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
    }
}

/**
 * Payload copy independence
 */

LUMEN_TEST( FMailBox_Payload, QueuedMessageIsIndependentFromSource )
{
    FMailBox Mailbox;

    constexpr FBooleanPayload SourcePayload{ .bFlag = true, .Value = 5 };

    FMessage Source = FMessage::Make( TestMessageType, TestSenderId, SourcePayload );

    Mailbox.Push( Source );

    Source.Sender = 1ULL;

    const TOptional<FMessage> Popped = Mailbox.Pop();

    LUMEN_CHECK_TRUE( Popped.HasValue() );
    LUMEN_CHECK_EQ( ( *Popped ).Sender, TestSenderId );
    LUMEN_CHECK_TRUE( ( *Popped ).GetPayload<FBooleanPayload>().bFlag );
    LUMEN_CHECK_EQ( ( *Popped ).GetPayload<FBooleanPayload>().Value, 5 );
}

LUMEN_TEST( FMailBox_Payload, DuplicatedPushYieldsIndependentCopies )
{
    FMailBox Mailbox;

    constexpr FBooleanPayload SourcePayload{ .bFlag = true, .Value = 5 };

    const FMessage Message = FMessage::Make( TestMessageType, TestSenderId, SourcePayload );

    Mailbox.Push( Message );
    Mailbox.Push( Message );

    const TOptional<FMessage> First  = Mailbox.Pop();
    const TOptional<FMessage> Second = Mailbox.Pop();

    LUMEN_CHECK_TRUE( First.HasValue() );
    LUMEN_CHECK_TRUE( Second.HasValue() );
    LUMEN_CHECK_EQ( ( *First ).GetPayload<FBooleanPayload>().Value, 5 );
    LUMEN_CHECK_EQ( ( *Second ).GetPayload<FBooleanPayload>().Value, 5 );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

/**
 * Volume
 */

LUMEN_TEST( FMailBox_Load, ExceedsDefaultFreeList )
{
    /**
     * INFO: Going beyond the mailbox's initial free-list pool must transparently
     *       allocate new nodes and keep the queue order intact.
     */
    FMailBox Mailbox;

    constexpr UInt32 MessageCount = 600U;

    for ( UInt32 Index = 0U; Index < MessageCount; ++Index )
    {
        Mailbox.Push( MakeTestMessage( Index, TestSenderId, static_cast<Int32>( Index ) ) );
    }

    LUMEN_CHECK_FALSE( Mailbox.IsEmpty() );

    UInt32 ReceivedCount = 0U;

    for ( ;; )
    {
        const TOptional<FMessage> Popped = Mailbox.Pop();

        if ( not Popped.HasValue() )
        {
            break;
        }

        LUMEN_CHECK_EQ( ( *Popped ).Type, TestMessageType + ReceivedCount );
        LUMEN_CHECK_EQ( ( *Popped ).GetPayload<Int32>(), static_cast<Int32>( ReceivedCount ) );

        ++ReceivedCount;
    }

    LUMEN_CHECK_EQ( ReceivedCount, MessageCount );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

LUMEN_TEST( FMailBox_Reserve, SupportsBurstWithoutAllocation )
{
    FMailBox Mailbox;

    Mailbox.Reserve( 512ULL );

    constexpr UInt32 MessageCount = 512U;

    for ( UInt32 Index = 0U; Index < MessageCount; ++Index )
    {
        Mailbox.Push( MakeTestMessage( Index, TestSenderId, static_cast<Int32>( Index ) ) );
    }

    LUMEN_CHECK_FALSE( Mailbox.IsEmpty() );

    UInt32 ReceivedCount = 0U;

    for ( ;; )
    {
        const TOptional<FMessage> Popped = Mailbox.Pop();

        if ( not Popped.HasValue() )
        {
            break;
        }

        LUMEN_CHECK_EQ( ( *Popped ).GetPayload<Int32>(), static_cast<Int32>( ReceivedCount ) );

        ++ReceivedCount;
    }

    LUMEN_CHECK_EQ( ReceivedCount, MessageCount );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

/**
 * Multithreading
 */

LUMEN_TEST( FMailBox_Threading, ManyProducersSingleConsumer )
{
    /**
     * INFO: The mailbox is a wait-free MPSC queue; every message pushed by any
     *       producer must reach the single consumer exactly once.
     */
    FMailBox Mailbox;
    TAtomic<UInt32> PushedCount = 0;
    UInt32 ReceivedCount        = 0U;

    constexpr UInt32 ThreadsCount        = 4U;
    constexpr UInt32 MessagesPerProducer = 250U;
    constexpr UInt32 TargetTotal         = ThreadsCount * MessagesPerProducer;

    FThread Producers[ThreadsCount];

    for ( UInt32 Index = 0U; Index < ThreadsCount; ++Index )
    {
        Producers[Index] = FThread( [&Mailbox, &PushedCount] ()
        {
            for ( UInt32 Message = 0U; Message < MessagesPerProducer; ++Message )
            {
                Mailbox.Push( MakeTestMessage( Message, TestSenderId, static_cast<Int32>( Message ) ) );
                PushedCount.fetch_add( 1U, std::memory_order_release );
            }
        } );
    }

    FThread Consumer( [&] ()
    {
        for ( ;; )
        {
            TOptional<FMessage> Popped = Mailbox.Pop();

            if ( Popped.HasValue() )
            {
                ++ReceivedCount;
                continue;
            }

            if ( PushedCount.load( std::memory_order_acquire ) >= TargetTotal and Mailbox.IsEmpty() )
            {
                return;
            }
        }
    } );

    for ( FThread &Producer : Producers )
    {
        Producer.Join();
    }

    Consumer.Join();

    LUMEN_CHECK_EQ( ReceivedCount, TargetTotal );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

LUMEN_TEST( FMailBox_Threading, AllProducedMessagesAreReceived )
{
    FMailBox Mailbox;
    UInt64 ReceivedCount = 0ULL;

    constexpr UInt32 ThreadsCount        = 4U;
    constexpr UInt32 MessagesPerProducer = 500U;
    constexpr UInt64 TargetTotal         = static_cast<UInt64>( ThreadsCount ) * MessagesPerProducer;

    FThread Producers[ThreadsCount];

    for ( UInt32 Index = 0U; Index < ThreadsCount; ++Index )
    {
        Producers[Index] = FThread( [Index, &Mailbox] ()
        {
            for ( UInt32 Message = 0U; Message < MessagesPerProducer; ++Message )
            {
                Mailbox.Push( MakeTestMessage( 0U, static_cast<ActorID>( Index ) + 1ULL, static_cast<Int32>( Index ) ) );
            }
        } );
    }

    for ( FThread &Producer : Producers )
    {
        Producer.Join();
    }

    for ( ;; )
    {
        const TOptional<FMessage> Popped = Mailbox.Pop();

        if ( not Popped.HasValue() )
        {
            break;
        }

        LUMEN_CHECK_NOT_EQ( ( *Popped ).Sender, 0ULL );
        ++ReceivedCount;
    }

    LUMEN_CHECK_EQ( ReceivedCount, TargetTotal );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

} // namespace Lumen
