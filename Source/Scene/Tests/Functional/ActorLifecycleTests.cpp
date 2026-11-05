/// @file ActorLifecycleTests.cpp
/// @brief Unit and concurrency tests for actor registration and lifetime.

#include <Actor/Actor.hpp>
#include <Tests/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <vector>

namespace Lumen
{

class FTestActor final : public AActor
{
    LUMEN_ACTOR_BODY( FTestActor )

public:

    explicit FTestActor ( const UInt32 InIndex          = 0U,
                          const USize InMailboxCapacity = DefaultMailboxCapacity,
                          TAtomic<Bool> *InDestroyed    = nullptr ) noexcept
        : AActor( InIndex, InMailboxCapacity ), DestroyedNotification( InDestroyed )
    {
    }

    ~FTestActor () noexcept override
    {
        if ( DestroyedNotification != nullptr )
        {
            DestroyedNotification->store( true, std::memory_order_release );
        }
    }

    void Receive ( const FMessage & ) noexcept override
    {
        const UInt32 Active = ActiveReceives.fetch_add( 1U, std::memory_order_acq_rel ) + 1U;
        UInt32 ObservedMax  = MaxActiveReceives.load( std::memory_order_relaxed );
        while ( ObservedMax < Active and not MaxActiveReceives.compare_exchange_weak( ObservedMax, Active ) )
        {
        }

        ReceivedCount.fetch_add( 1U, std::memory_order_release );
        if ( bBlockReceive.load( std::memory_order_acquire ) )
        {
            bEnteredReceive.store( true, std::memory_order_release );
            while ( not bReleaseReceive.load( std::memory_order_acquire ) )
            {
                std::this_thread::yield();
            }
        }

        ActiveReceives.fetch_sub( 1U, std::memory_order_release );
    }

public:

    TAtomic<UInt32> ReceivedCount{ 0U };
    TAtomic<UInt32> ActiveReceives{ 0U };
    TAtomic<UInt32> MaxActiveReceives{ 0U };
    TAtomic<Bool> bBlockReceive{ false };
    TAtomic<Bool> bEnteredReceive{ false };
    TAtomic<Bool> bReleaseReceive{ false };

private:

    TAtomic<Bool> *DestroyedNotification = nullptr;
};

class FOtherTestActor final : public AActor
{
    LUMEN_ACTOR_BODY( FOtherTestActor )

public:

    void Receive ( const FMessage & ) noexcept override
    {
        ///
    }
};

static_assert( Concepts::CActor<FTestActor> );

LUMEN_TEST( ActorLifecycle_Registration, ActorTypesHaveDistinctIds )
{
    FTestActor Actor;
    FOtherTestActor OtherActor;

    LUMEN_CHECK_NE( Actor.GetTypeID(), OtherActor.GetTypeID() );
    LUMEN_CHECK_EQ( Actor.GetTypeID(), FTestActor().GetTypeID() );
}

LUMEN_TEST( ActorLifecycle_Generation, GenerationsCanCoexistAtSameIndex )
{
    constexpr UInt32 Index = 9221U;
    FTestActor First( Index );
    FTestActor Second( Index );

    LUMEN_CHECK_EQ( First.Initialize(), EActorRegistrationResult::Success );
    LUMEN_CHECK_EQ( Second.Initialize(), EActorRegistrationResult::Success );
    LUMEN_CHECK_EQ( First.GetId().Index, Second.GetId().Index );
    LUMEN_CHECK_NE( First.GetId().Generation, Second.GetId().Generation );

    const FActorRef FirstRef  = First.GetRef();
    const FActorRef SecondRef = Second.GetRef();
    LUMEN_CHECK_EQ( FirstRef.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );
    LUMEN_CHECK_EQ( SecondRef.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );
    LUMEN_CHECK_EQ( First.ProcessMailbox( 1U ), 1U );
    LUMEN_CHECK_EQ( Second.ProcessMailbox( 1U ), 1U );
    LUMEN_CHECK_EQ( First.ReceivedCount.load(), 1U );
    LUMEN_CHECK_EQ( Second.ReceivedCount.load(), 1U );

    First.Shutdown();
    LUMEN_CHECK_EQ( SecondRef.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );
    LUMEN_CHECK_EQ( Second.ProcessMailbox( 1U ), 1U );
    Second.Shutdown();
}

LUMEN_TEST( ActorLifecycle_Registration, RequiresExplicitPublication )
{
    FTestActor *Actor              = new FTestActor();
    const FActorRef UnpublishedRef = Actor->GetRef();

    LUMEN_CHECK_FALSE( UnpublishedRef.IsValid() );
    LUMEN_CHECK_EQ( UnpublishedRef.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::ActorUnavailable );
    LUMEN_CHECK_EQ( Actor->Initialize(), EActorRegistrationResult::Success );
    LUMEN_CHECK_TRUE( Actor->GetId().IsValid() );
    LUMEN_CHECK_TRUE( Actor->IsActive() );
    LUMEN_CHECK_EQ( Actor->Initialize(), EActorRegistrationResult::AlreadyRegistered );

    const FActorRef Ref = Actor->GetRef();
    LUMEN_CHECK_EQ( Ref.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );
    LUMEN_CHECK_EQ( Actor->ProcessMailbox( 1U ), 1U );
    LUMEN_CHECK_EQ( Actor->ReceivedCount.load(), 1U );

    Actor->Destroy();
    LUMEN_CHECK_EQ( Ref.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::ActorUnavailable );
}

LUMEN_TEST( ActorLifecycle_Generation, StaleReferenceCannotTargetReplacement )
{
    constexpr UInt32 Slot = 9137U;

    FTestActor *FirstActor = new FTestActor( Slot );
    LUMEN_CHECK_EQ( FirstActor->Initialize(), EActorRegistrationResult::Success );
    const FActorRef StaleRef = FirstActor->GetRef();
    FirstActor->Destroy();

    FTestActor *Replacement = new FTestActor( Slot );
    LUMEN_CHECK_EQ( Replacement->Initialize(), EActorRegistrationResult::Success );
    const FActorRef ReplacementRef = Replacement->GetRef();

    LUMEN_CHECK_EQ( ReplacementRef.GetId().Index, StaleRef.GetId().Index );
    LUMEN_CHECK_NE( ReplacementRef.GetId().Generation, StaleRef.GetId().Generation );
    LUMEN_CHECK_EQ( StaleRef.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::ActorUnavailable );
    LUMEN_CHECK_EQ( ReplacementRef.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );
    LUMEN_CHECK_EQ( Replacement->ProcessMailbox(), 1U );
    LUMEN_CHECK_EQ( Replacement->ReceivedCount.load(), 1U );

    Replacement->Destroy();
}

LUMEN_TEST( ActorLifecycle_Registration, ConcurrentInitializeAndShutdown )
{
    constexpr UInt32 IterationCount = 256U;
    for ( UInt32 Iteration = 0U; Iteration < IterationCount; ++Iteration )
    {
        FTestActor Actor;
        TAtomic<Bool> bStart{ false };
        EActorRegistrationResult Registration = EActorRegistrationResult::AlreadyRegistered;

        std::thread Initializer(
            [&]
            {
                while ( not bStart.load( std::memory_order_acquire ) )
                {
                    std::this_thread::yield();
                }
                Registration = Actor.Initialize();
            } );
        std::thread Stopper(
            [&]
            {
                while ( not bStart.load( std::memory_order_acquire ) )
                {
                    std::this_thread::yield();
                }
                Actor.Shutdown();
            } );

        bStart.store( true, std::memory_order_release );
        Initializer.join();
        Stopper.join();

        LUMEN_CHECK_TRUE( Registration == EActorRegistrationResult::Success ||
                          Registration == EActorRegistrationResult::ActorStopped );
        LUMEN_CHECK_EQ( Actor.GetId().IsValid(), Registration == EActorRegistrationResult::Success );
        LUMEN_CHECK_FALSE( Actor.IsActive() );
    }
}

LUMEN_TEST( ActorLifecycle_Registration, StoppedActorCannotRestart )
{
    FTestActor Actor;

    LUMEN_CHECK_EQ( Actor.Initialize(), EActorRegistrationResult::Success );
    Actor.Shutdown();
    LUMEN_CHECK_EQ( Actor.Initialize(), EActorRegistrationResult::ActorStopped );
    LUMEN_CHECK_FALSE( Actor.IsActive() );
    LUMEN_CHECK_EQ( Actor.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Closed );
}

LUMEN_TEST( ActorLifecycle_Backpressure, ActorRefPropagatesMailboxResult )
{
    FTestActor Actor( 0U, 1ULL );
    LUMEN_CHECK_EQ( Actor.Initialize(), EActorRegistrationResult::Success );
    const FActorRef Ref = Actor.GetRef();

    LUMEN_CHECK_EQ( Ref.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );
    LUMEN_CHECK_EQ( Ref.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Full );
    LUMEN_CHECK_EQ( Actor.ProcessMailbox( 1U ), 1U );
    LUMEN_CHECK_EQ( Ref.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );
    Actor.Shutdown();
}

LUMEN_TEST( ActorLifecycle_Processing, EnforcesFiniteAndExplicitUnlimitedBudgets )
{
    FTestActor Actor;
    LUMEN_CHECK_EQ( Actor.Initialize(), EActorRegistrationResult::Success );

    for ( UInt32 Index = 0U; Index < 5U; ++Index )
    {
        LUMEN_CHECK_EQ( Actor.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );
    }

    LUMEN_CHECK_EQ( Actor.ProcessMailbox( 2U ), 2U );
    LUMEN_CHECK_EQ( Actor.ReceivedCount.load(), 2U );
    LUMEN_CHECK_EQ( Actor.ProcessMailbox( 0U ), 3U );
    LUMEN_CHECK_EQ( Actor.ReceivedCount.load(), 5U );

    for ( UInt32 Index = 0U; Index < AActor::DefaultProcessingBudget + 1U; ++Index )
    {
        LUMEN_CHECK_EQ( Actor.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );
    }

    LUMEN_CHECK_EQ( Actor.ProcessMailbox(), AActor::DefaultProcessingBudget );
    LUMEN_CHECK_EQ( Actor.ProcessMailbox( 1U ), 1U );
    Actor.Shutdown();
}

LUMEN_TEST( ActorLifecycle_Processing, ReceiveIsExclusive )
{
    FTestActor Actor;
    LUMEN_CHECK_EQ( Actor.Initialize(), EActorRegistrationResult::Success );
    Actor.bBlockReceive.store( true );
    LUMEN_CHECK_EQ( Actor.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );

    UInt32 FirstProcessedCount = 0U;
    std::thread FirstProcessor( [&] { FirstProcessedCount = Actor.ProcessMailbox( 0U ); } );

    while ( not Actor.bEnteredReceive.load( std::memory_order_acquire ) )
    {
        std::this_thread::yield();
    }

    LUMEN_CHECK_EQ( Actor.ProcessMailbox( 10U ), 0U );
    LUMEN_CHECK_EQ( Actor.MaxActiveReceives.load(), 1U );

    Actor.bReleaseReceive.store( true, std::memory_order_release );
    FirstProcessor.join();

    LUMEN_CHECK_EQ( FirstProcessedCount, 1U );
    LUMEN_CHECK_EQ( Actor.ReceivedCount.load(), 1U );
    Actor.Shutdown();
}

LUMEN_TEST( ActorLifecycle_Processing, ShutdownWaitsForActiveReceive )
{
    FTestActor Actor;
    LUMEN_CHECK_EQ( Actor.Initialize(), EActorRegistrationResult::Success );
    Actor.bBlockReceive.store( true );
    LUMEN_CHECK_EQ( Actor.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );

    std::thread Processor( [&] { Actor.ProcessMailbox( 0U ); } );
    while ( not Actor.bEnteredReceive.load( std::memory_order_acquire ) )
    {
        std::this_thread::yield();
    }

    TAtomic<Bool> bShutdownReturned{ false };
    std::thread Stopper(
        [&]
        {
            Actor.Shutdown();
            bShutdownReturned.store( true, std::memory_order_release );
        } );

    LUMEN_CHECK_FALSE( bShutdownReturned.load( std::memory_order_acquire ) );
    Actor.bReleaseReceive.store( true, std::memory_order_release );
    Processor.join();
    Stopper.join();

    LUMEN_CHECK_TRUE( bShutdownReturned.load( std::memory_order_acquire ) );
    LUMEN_CHECK_EQ( Actor.ReceivedCount.load(), 1U );
}

LUMEN_TEST( ActorLifecycle_Destruction, WaitsForActiveReceiveBeforeDeletingDerivedActor )
{
    TAtomic<Bool> bDestroyed{ false };
    FTestActor *Actor = new FTestActor( 0U, AActor::DefaultMailboxCapacity, &bDestroyed );
    LUMEN_CHECK_EQ( Actor->Initialize(), EActorRegistrationResult::Success );
    Actor->bBlockReceive.store( true );
    LUMEN_CHECK_EQ( Actor->EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::Success );
    const FActorRef Ref = Actor->GetRef();

    std::thread Processor( [&] { Actor->ProcessMailbox( 0U ); } );
    while ( not Actor->bEnteredReceive.load( std::memory_order_acquire ) )
    {
        std::this_thread::yield();
    }

    TAtomic<Bool> bDestroyReturned{ false };
    std::thread Destroyer(
        [&]
        {
            Actor->Destroy();
            bDestroyReturned.store( true, std::memory_order_release );
        } );

    while ( Ref.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ) != EEnqueueResult::ActorUnavailable )
    {
        std::this_thread::yield();
    }

    LUMEN_CHECK_FALSE( bDestroyReturned.load( std::memory_order_acquire ) );
    LUMEN_CHECK_FALSE( bDestroyed.load( std::memory_order_acquire ) );

    Actor->bReleaseReceive.store( true, std::memory_order_release );
    Processor.join();
    Destroyer.join();

    LUMEN_CHECK_TRUE( bDestroyReturned.load() );
    LUMEN_CHECK_TRUE( bDestroyed.load() );
    LUMEN_CHECK_EQ( Ref.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::ActorUnavailable );
}

LUMEN_TEST( ActorLifecycle_Destruction, ConcurrentSendersAreDrainedBeforeDeletion )
{
    FTestActor *Actor = new FTestActor( 0U, 0ULL );
    LUMEN_CHECK_EQ( Actor->Initialize(), EActorRegistrationResult::Success );
    const FActorRef Ref = Actor->GetRef();

    constexpr UInt32 SenderCount = 4U;
    TAtomic<Bool> bStart{ false };
    TAtomic<UInt32> StartedSenders{ 0U };
    TAtomic<Bool> bUnexpectedFailure{ false };
    std::vector<std::thread> Senders;

    for ( UInt32 Sender = 0U; Sender < SenderCount; ++Sender )
    {
        Senders.emplace_back(
            [&]
            {
                while ( not bStart.load( std::memory_order_acquire ) )
                {
                    std::this_thread::yield();
                }

                for ( UInt32 Index = 0U; Index < 2000U; ++Index )
                {
                    const EEnqueueResult Result = Ref.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) );
                    if ( Result == EEnqueueResult::ActorUnavailable || Result == EEnqueueResult::Closed )
                    {
                        break;
                    }
                    if ( Result != EEnqueueResult::Success )
                    {
                        bUnexpectedFailure.store( true, std::memory_order_release );
                        break;
                    }
                    if ( Index == 0U )
                    {
                        StartedSenders.fetch_add( 1U, std::memory_order_acq_rel );
                    }
                }
            } );
    }

    bStart.store( true, std::memory_order_release );
    while ( StartedSenders.load( std::memory_order_acquire ) < SenderCount )
    {
        std::this_thread::yield();
    }
    Actor->Destroy();

    for ( std::thread &Sender : Senders )
    {
        Sender.join();
    }

    LUMEN_CHECK_FALSE( bUnexpectedFailure.load() );
    LUMEN_CHECK_EQ( Ref.EnqueueMessage( FMessage::Make( 1U, ActorID{} ) ), EEnqueueResult::ActorUnavailable );
}

} // namespace Lumen
