/*
 * @file ActorTests.cpp
 * @brief Functional tests for AActor.
 */

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>

#include <Messages/EngineMessageTypes.hpp>

#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

#include <thread>
#include <type_traits>

namespace Lumen
{

/**
 * Test actor fixtures
 */

/** @brief Actor recording every received message for inspection. */
class FTestActor final : public AActor
{
    LUMEN_ACTOR_BODY ( FTestActor );

public:

    explicit FTestActor ( const ActorID InId ) noexcept : AActor( InId )
    {
        /**
         *
         */
    }

    void Receive ( const FMessage &InMessage ) noexcept override
    {
        ReceivedMessages.push_back( InMessage );
    }

    /** @brief Exposes the protected mailbox reservation for tests. */
    void ReserveMailbox ( const USize InCapacity ) noexcept
    {
        GetMailbox().Reserve( InCapacity );
    }

    [[nodiscard]] const TVector<FMessage> &GetReceivedMessages () const noexcept
    {
        return ReceivedMessages;
    }

private:

    TVector<FMessage> ReceivedMessages;
};

/** @brief Distinct actor type used to observe per-type identity. */
class FAltTestActor final : public AActor
{
    LUMEN_ACTOR_BODY ( FAltTestActor );

public:

    explicit FAltTestActor ( const ActorID InId ) noexcept : AActor( InId )
    {
        /**
         *
         */
    }

    void Receive ( const FMessage &InMessage ) noexcept override
    {
        ReceivedTypes.push_back( InMessage.Type );
    }

    [[nodiscard]] const TVector<UInt32> &GetReceivedTypes () const noexcept
    {
        return ReceivedTypes;
    }

private:

    TVector<UInt32> ReceivedTypes;
};

/** @brief Type failing the CActor concept (wrong Receive signature). */
struct FInvalidActor
{
    void Receive ( const Int32 InCode )
    {
        ReceiveCount += InCode;
    }

    Int32 ReceiveCount = 0;
};

/**
 * Construction
 */

LUMEN_TEST( AActor_Construct, AssignsProvidedId )
{
    const ActorID ActorId = Internal::FActorRegistry::AllocateId();

    const FTestActor Actor( ActorId );

    LUMEN_CHECK_EQ( Actor.GetId(), ActorId );
}

LUMEN_TEST( AActor_Construct, GetRefMatchesActorId )
{
    const ActorID ActorId = Internal::FActorRegistry::AllocateId();

    const FTestActor Actor( ActorId );
    const FActorRef ActorRef = Actor.GetRef();

    LUMEN_CHECK_EQ( ActorRef.GetId(), ActorId );
    LUMEN_CHECK_TRUE( ActorRef.IsValid() );
}

/**
 * Type identity
 */

LUMEN_TEST( AActor_TypeID, UniqueAcrossActorTypes )
{
    /**
     * INFO: LUMEN_ACTOR_BODY binds GetTypeID to the compile-time registry of
     *       each concrete actor type.
     */
    const FTestActor Actor( Internal::FActorRegistry::AllocateId() );
    const FAltTestActor AltActor( Internal::FActorRegistry::AllocateId() );

    LUMEN_CHECK_NOT_EQ( Actor.GetTypeID(), 0ULL );
    LUMEN_CHECK_NOT_EQ( AltActor.GetTypeID(), 0ULL );
    LUMEN_CHECK_NOT_EQ( Actor.GetTypeID(), AltActor.GetTypeID() );
}

LUMEN_TEST( AActor_TypeID, StableAcrossSameTypeInstances )
{
    const FTestActor FirstActor( Internal::FActorRegistry::AllocateId() );
    const FTestActor SecondActor( Internal::FActorRegistry::AllocateId() );

    LUMEN_CHECK_EQ( FirstActor.GetTypeID(), SecondActor.GetTypeID() );
}

LUMEN_TEST( AActor_TypeID, NegativeRegistryContract )
{
    static_assert( Concepts::CActor<FTestActor> );
    static_assert( Concepts::CActor<FAltTestActor> );
    static_assert( not Concepts::CActor<FInvalidActor> );
    static_assert( not Concepts::CActor<Int32> );

    LUMEN_CHECK_TRUE( Concepts::CActor<FTestActor> );
    LUMEN_CHECK_FALSE( Concepts::CActor<FInvalidActor> );
}

/**
 * Mailbox processing
 */

LUMEN_TEST( AActor_MailboxProcessing, DeliversEveryPendingMessageInOrder )
{
    /**
     * INFO: RunFrame is the public single-frame entry point stopping just short
     *       of the tick: it processes the mailbox with an explicit budget (0 =
     *       all pending messages) before invoking the default no-op OnTick.
     */
    FTestActor Actor( Internal::FActorRegistry::AllocateId() );

    for ( UInt32 Index = 0U; Index < 16U; ++Index )
    {
        Actor.EnqueueMessage( FMessage::Make( Index, 0ULL, static_cast<Int32>( Index ) ) );
    }

    Actor.RunFrame( 0.0, 0U );

    const TVector<FMessage> &Received = Actor.GetReceivedMessages();

    LUMEN_CHECK_EQ( Received.size(), 16ULL );

    for ( UInt32 ExpectedIndex = 0U; ExpectedIndex < 16U; ++ExpectedIndex )
    {
        LUMEN_CHECK_EQ( Received[ExpectedIndex].Type, ExpectedIndex );
        LUMEN_CHECK_EQ( Received[ExpectedIndex].GetPayload<Int32>(), static_cast<Int32>( ExpectedIndex ) );
    }
}

LUMEN_TEST( AActor_MailboxProcessing, HonorsPartialBudgetThenResumes )
{
    FTestActor Actor( Internal::FActorRegistry::AllocateId() );

    for ( UInt32 Index = 0U; Index < 10U; ++Index )
    {
        Actor.EnqueueMessage( FMessage::Make( Index, 0ULL ) );
    }

    Actor.RunFrame( 0.0, 4U );

    LUMEN_CHECK_EQ( Actor.GetReceivedMessages().size(), 4ULL );

    Actor.RunFrame( 0.0, 2U );

    LUMEN_CHECK_EQ( Actor.GetReceivedMessages().size(), 6ULL );

    Actor.RunFrame( 0.0, 0U );

    LUMEN_CHECK_EQ( Actor.GetReceivedMessages().size(), 10ULL );

    const TVector<FMessage> &Received = Actor.GetReceivedMessages();

    for ( UInt32 ExpectedIndex = 0U; ExpectedIndex < 10U; ++ExpectedIndex )
    {
        LUMEN_CHECK_EQ( Received[ExpectedIndex].Type, ExpectedIndex );
    }
}

LUMEN_TEST( AActor_MailboxProcessing, EmptyMailboxProcessesNothing )
{
    FTestActor Actor( Internal::FActorRegistry::AllocateId() );

    Actor.RunFrame( 0.0, 0U );
    Actor.RunFrame( 0.0, 3U );

    LUMEN_CHECK_EQ( Actor.GetReceivedMessages().size(), 0ULL );
}

LUMEN_TEST( AActor_MailboxProcessing, DeliversTypedPayload )
{
    FTestActor Actor( Internal::FActorRegistry::AllocateId() );

    constexpr FTickPayload SourcePayload{ .DeltaTime = 33.3 };
    constexpr UInt32 TickType = static_cast<UInt32>( EEngineMessage::Tick );

    Actor.EnqueueMessage( FMessage::Make( TickType, 400ULL, SourcePayload ) );
    Actor.RunFrame( 0.0, 0U );

    const TVector<FMessage> &Received = Actor.GetReceivedMessages();

    LUMEN_CHECK_EQ( Received.size(), 1ULL );
    LUMEN_CHECK_EQ( Received[0].Sender, 400ULL );
    LUMEN_CHECK_EQ( Received[0].Type, TickType );
    LUMEN_CHECK_TRUE( Received[0].GetPayload<FTickPayload>().DeltaTime == 33.3 );
}

/**
 * Mailbox queuing
 */

LUMEN_TEST( AActor_MailboxQueuing, QueuedCopyDetachesFromSource )
{
    FTestActor Actor( Internal::FActorRegistry::AllocateId() );

    FMessage Mutable = FMessage::Make( 11U, 22ULL, 44 );

    Actor.EnqueueMessage( Mutable );

    Mutable.Sender = 1ULL;
    Mutable.Type   = 99U;

    Actor.RunFrame( 0.0, 0U );

    const TVector<FMessage> &Received = Actor.GetReceivedMessages();

    LUMEN_CHECK_EQ( Received.size(), 1ULL );
    LUMEN_CHECK_EQ( Received[0].Sender, 22ULL );
    LUMEN_CHECK_EQ( Received[0].Type, 11U );
    LUMEN_CHECK_EQ( Received[0].GetPayload<Int32>(), 44 );
    LUMEN_CHECK_EQ( Mutable.Sender, 1ULL );
    LUMEN_CHECK_EQ( Mutable.Type, 99U );
}

LUMEN_TEST( AActor_MailboxQueuing, ReserveHandlesBurstWithoutAllocation )
{
    FTestActor Actor( Internal::FActorRegistry::AllocateId() );

    Actor.ReserveMailbox( 64ULL );

    for ( UInt32 Index = 0U; Index < 64U; ++Index )
    {
        Actor.EnqueueMessage( FMessage::Make( Index, 0ULL ) );
    }

    Actor.RunFrame( 0.0, 0U );

    LUMEN_CHECK_EQ( Actor.GetReceivedMessages().size(), 64ULL );
}

/**
 * Lifetime
 */

LUMEN_TEST( AActor_Lifetime, DestructionWithPendingMessagesIsSound )
{
    const ActorID ActorId = Internal::FActorRegistry::AllocateId();

    {
        FTestActor Actor( ActorId );

        Actor.EnqueueMessage( FMessage::Make( 1U, 0ULL ) );
        Actor.EnqueueMessage( FMessage::Make( 2U, 0ULL ) );
    }

    /**
     * INFO: Unregistering releases the slot and bumps its generation, so the dead
     *       ID can never be respawned as-is: a fresh allocation hands back the same
     *       slot carrying the bumped generation.
     */
    const ActorID RebornId = Internal::FActorRegistry::AllocateId();

    FTestActor RebornActor( RebornId );

    RebornActor.EnqueueMessage( FMessage::Make( 3U, 0ULL ) );
    RebornActor.RunFrame( 0.0, 0U );

    LUMEN_CHECK_EQ( ActorIdSlotIndex( RebornId ), ActorIdSlotIndex( ActorId ) );
    LUMEN_CHECK_NOT_EQ( RebornId, ActorId );

    const TVector<FMessage> &Received = RebornActor.GetReceivedMessages();

    LUMEN_CHECK_EQ( Received.size(), 1ULL );
    LUMEN_CHECK_EQ( Received[0].Type, 3U );
}

LUMEN_TEST( AActor_Lifetime, TraitsSupportPolymorphicOwnership )
{
    static_assert( not std::is_copy_constructible_v<FTestActor> );
    static_assert( not std::is_copy_assignable_v<FTestActor> );
    static_assert( not std::is_move_constructible_v<FTestActor> );
    static_assert( not std::is_move_assignable_v<FTestActor> );
    static_assert( std::is_abstract_v<AActor> );
    static_assert( std::has_virtual_destructor_v<AActor> );
    static_assert( std::is_polymorphic_v<AActor> );

    LUMEN_CHECK_FALSE( std::is_copy_constructible_v<FTestActor> );
    LUMEN_CHECK_TRUE( std::has_virtual_destructor_v<AActor> );
}

/**
 * Threading
 */

LUMEN_TEST( AActor_Threading, MultipleProducersEnqueueToSingleActor )
{
    /**
     * INFO: EnqueueMessage routes into the MPSC mailbox, so any number of
     *       producers may push simultaneously without loss.
     */
    FTestActor Actor( Internal::FActorRegistry::AllocateId() );
    TAtomic<UInt32> PushedCount = 0;

    constexpr UInt32 ThreadsCount        = 4U;
    constexpr UInt32 MessagesPerProducer = 100U;
    constexpr UInt32 TargetTotal         = ThreadsCount * MessagesPerProducer;

    std::thread Producers[ThreadsCount];

    for ( UInt32 Index = 0U; Index < ThreadsCount; ++Index )
    {
        Producers[Index] = std::thread( [&Actor, &PushedCount, Index] ()
        {
            for ( UInt32 Message = 0U; Message < MessagesPerProducer; ++Message )
            {
                Actor.EnqueueMessage( FMessage::Make( static_cast<UInt32>( Index ), 0ULL ) );
                PushedCount.fetch_add( 1U, std::memory_order_relaxed );
            }
        } );
    }

    for ( std::thread &Producer : Producers )
    {
        Producer.join();
    }

    LUMEN_CHECK_EQ( PushedCount.load( std::memory_order_relaxed ), TargetTotal );

    Actor.RunFrame( 0.0, 0U );

    LUMEN_CHECK_EQ( Actor.GetReceivedMessages().size(), TargetTotal );
}

} // namespace Lumen
