/*
 * @file WorldTests.cpp
 * @brief Functional tests for FWorld.
 */

#include <World/World.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>
#include <Memory/SharedPtr.hpp>
#include <Messages/EngineMessageTypes.hpp>
#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

#include <type_traits>

namespace Lumen
{

/**
 * Test actor fixtures
 */

/** @brief Actor counting processed messages, distinguishing tick furniture. */
class FWorldTestActor final : public AActor
{
    LUMEN_ACTOR_BODY ( FWorldTestActor );

public:

    explicit FWorldTestActor ( const ActorID InId ) noexcept : AActor( InId )
    {
        /* */
    }

    void Receive ( const FMessage &InMessage ) noexcept override
    {
        ReceivedTypes.push_back( InMessage.Type );
        ReceivedPayloads.push_back( InMessage.GetPayload<Int32>() );
    }

    void OnTick ( const Float64 InDeltaTime ) noexcept override
    {
        TickDelta = InDeltaTime;
        ++TickCount;
    }

    [[nodiscard]] const TVector<UInt32> &GetReceivedTypes () const noexcept
    {
        return ReceivedTypes;
    }

    [[nodiscard]] const TVector<Int32> &GetReceivedPayloads () const noexcept
    {
        return ReceivedPayloads;
    }

    [[nodiscard]] Float64 GetTickDelta () const noexcept
    {
        return TickDelta;
    }

    [[nodiscard]] UInt32 GetTickCount () const noexcept
    {
        return TickCount;
    }

private:

    TVector<UInt32> ReceivedTypes;
    TVector<Int32> ReceivedPayloads;
    Float64 TickDelta = -1.0;
    UInt32 TickCount  = 0U;
};

/** @brief Second actor type used to exercise type diversity inside a world. */
class FWorldAltActor final : public AActor
{
    LUMEN_ACTOR_BODY ( FWorldAltActor );

public:

    explicit FWorldAltActor ( const ActorID InId ) noexcept : AActor( InId )
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

/** @brief Actor taking a custom constructor forwarding through SpawnActor. */
class FForwardTestActor final : public AActor
{
    LUMEN_ACTOR_BODY ( FForwardTestActor );

public:

    FForwardTestActor ( const ActorID InId, const UInt32 InConstant ) noexcept : AActor( InId ), ForwardedConstant( InConstant )
    {
        /**
         *
         */
    }

    void Receive ( const FMessage &InMessage ) noexcept override
    {
        ReceivedTypes.push_back( InMessage.Type );
    }

    [[nodiscard]] UInt32 GetForwardedConstant () const noexcept
    {
        return ForwardedConstant;
    }

private:

    TVector<UInt32> ReceivedTypes;
    UInt32 ForwardedConstant = 0U;
};

/**
 * Constants & traits
 */

LUMEN_TEST( FWorld_Constant, MaxMessagesPerTickContract )
{
    LUMEN_CHECK_EQ( FWorld::MaxMessagesPerTick, 100U );
}

LUMEN_TEST( FWorld_Traits, CannotBeCopiedOrMoved )
{
    static_assert( not std::is_copy_constructible_v<FWorld> );
    static_assert( not std::is_copy_assignable_v<FWorld> );
    static_assert( not std::is_move_constructible_v<FWorld> );
    static_assert( not std::is_move_assignable_v<FWorld> );
    static_assert( std::has_virtual_destructor_v<FWorld> );

    LUMEN_CHECK_FALSE( std::is_copy_constructible_v<FWorld> );
    LUMEN_CHECK_TRUE( std::has_virtual_destructor_v<FWorld> );
}

/**
 * Empty world
 */

LUMEN_TEST( FWorld_EmptyWorld, BroadcastAndTickAreHarmless )
{
    FWorld World;

    World.Broadcast( FMessage::Make( 1U, 0ULL ) );
    World.Tick( 0.0 );
    World.Tick( -1.0 );
}

/**
 * SpawnActor
 */

LUMEN_TEST( FWorld_SpawnActor, AssignsSequentialIds )
{
    FWorld World;

    const TSharedRef<FWorldTestActor> First  = World.SpawnActor<FWorldTestActor>();
    const TSharedRef<FWorldTestActor> Second = World.SpawnActor<FWorldTestActor>();
    const TSharedRef<FWorldTestActor> Third  = World.SpawnActor<FWorldTestActor>();

    /**
     * INFO: Actor ids come from the process-wide registry counter; only the
     *       relative order within one world is deterministic.
     */
    const ActorID FirstId  = First->GetId();
    const ActorID SecondId = Second->GetId();
    const ActorID ThirdId  = Third->GetId();

    LUMEN_CHECK_NOT_EQ( FirstId, 0ULL );
    LUMEN_CHECK_NOT_EQ( SecondId, 0ULL );
    LUMEN_CHECK_NOT_EQ( ThirdId, 0ULL );

    LUMEN_CHECK_NOT_EQ( FirstId, SecondId );
    LUMEN_CHECK_NOT_EQ( SecondId, ThirdId );
    LUMEN_CHECK_NOT_EQ( FirstId, ThirdId );

    LUMEN_CHECK_NOT_EQ( Lumen::ActorIdSlotIndex( FirstId ), Lumen::ActorIdSlotIndex( SecondId ) );
    LUMEN_CHECK_NOT_EQ( Lumen::ActorIdSlotIndex( SecondId ), Lumen::ActorIdSlotIndex( ThirdId ) );

    LUMEN_CHECK_TRUE( First->GetRef().IsValid() );
}

LUMEN_TEST( FWorld_SpawnActor, SharesOwnershipWithWorld )
{
    FWorld World;

    const TSharedRef<FWorldTestActor> SpawnedRef = World.SpawnActor<FWorldTestActor>();

    /**
     * INFO: Three strong owners exist: the test reference plus the two entries
     *       the world stores in its Actors map and ActiveActors vector.
     */
    LUMEN_CHECK_EQ( SpawnedRef.GetSharedReferenceCount(), 3ULL );
}

LUMEN_TEST( FWorld_SpawnActor, ActorOutlivesWorldUntilReferencesRelease )
{
    /**
     * INFO: The world holds actors through shared ownership; destroying the
     *       world releases its reference only, keeping the actor processable.
     */
    const TSharedRef<FWorldTestActor> FloatingActor = [] () -> TSharedRef<FWorldTestActor>
    {
        FWorld World;

        const TSharedRef<FWorldTestActor> Spawned = World.SpawnActor<FWorldTestActor>();

        Spawned->EnqueueMessage( FMessage::Make( 7U, 0ULL ) );

        LUMEN_CHECK_EQ( Spawned.GetSharedReferenceCount(), 3ULL );

        return Spawned;
    }();

    LUMEN_CHECK_EQ( FloatingActor.GetSharedReferenceCount(), 1ULL );

    FloatingActor->RunFrame( 0.0, 0U );

    const TVector<UInt32> &Received = FloatingActor->GetReceivedTypes();

    LUMEN_CHECK_EQ( Received.size(), 1ULL );
    LUMEN_CHECK_EQ( Received[0], 7U );
}

LUMEN_TEST( FWorld_SpawnActor, ForwardsCustomConstructorArguments )
{
    /**
     * INFO: SpawnActor prepends the generated id and forwards every extra
     *       argument to the actor constructor.
     */
    FWorld World;

    const TSharedRef<FForwardTestActor> Spawned = World.SpawnActor<FForwardTestActor>( 41U );

    LUMEN_CHECK_NOT_EQ( Spawned->GetId(), 0ULL );
    LUMEN_CHECK_EQ( Spawned->GetForwardedConstant(), 41U );
}

/**
 * Broadcast
 */

LUMEN_TEST( FWorld_Broadcast, QueuesForEverySpawnedActor )
{
    FWorld World;

    const TSharedRef<FWorldTestActor> First  = World.SpawnActor<FWorldTestActor>();
    const TSharedRef<FWorldTestActor> Second = World.SpawnActor<FWorldTestActor>();
    const TSharedRef<FWorldAltActor> Alt     = World.SpawnActor<FWorldAltActor>();

    constexpr UInt32 BroadcastType = 4242U;
    constexpr Int32 BroadcastMark  = 9;

    const FMessage BroadcastMsg = FMessage::Make( BroadcastType, 111ULL, BroadcastMark );
    World.Broadcast( BroadcastMsg );

    /** INFO: Enqueueing does not process; the mailbox holds the message. */
    LUMEN_CHECK_EQ( First->GetReceivedTypes().size(), 0ULL );
    LUMEN_CHECK_EQ( Second->GetReceivedTypes().size(), 0ULL );
    LUMEN_CHECK_EQ( Alt->GetReceivedTypes().size(), 0ULL );

    First->RunFrame( 0.0, 0U );
    Second->RunFrame( 0.0, 0U );
    Alt->RunFrame( 0.0, 0U );

    LUMEN_CHECK_EQ( First->GetReceivedTypes().size(), 1ULL );
    LUMEN_CHECK_EQ( Second->GetReceivedTypes().size(), 1ULL );
    LUMEN_CHECK_EQ( Alt->GetReceivedTypes().size(), 1ULL );

    LUMEN_CHECK_EQ( First->GetReceivedTypes()[0], BroadcastType );
    LUMEN_CHECK_EQ( Second->GetReceivedTypes()[0], BroadcastType );
    LUMEN_CHECK_EQ( Alt->GetReceivedTypes()[0], BroadcastType );

    LUMEN_CHECK_EQ( First->GetReceivedPayloads()[0], BroadcastMark );
    LUMEN_CHECK_EQ( Second->GetReceivedPayloads()[0], BroadcastMark );
}

/**
 * Tick
 */

LUMEN_TEST( FWorld_Tick, DeliversTickPayloadToActors )
{
    FWorld World;

    const TSharedRef<FWorldTestActor> Actor = World.SpawnActor<FWorldTestActor>();

    World.Tick( 16.0 );

    LUMEN_CHECK_EQ( Actor->GetTickDelta(), 16.0 );
    LUMEN_CHECK_EQ( Actor->GetTickCount(), 1U );
    LUMEN_CHECK_EQ( Actor->GetReceivedTypes().size(), 0ULL );
}

LUMEN_TEST( FWorld_Tick, ThrottlesMailboxThroughputPerActor )
{
    FWorld World;

    const TSharedRef<FWorldTestActor> Actor = World.SpawnActor<FWorldTestActor>();

    constexpr UInt32 PreloadedCount = 150U;

    for ( UInt32 Index = 0U; Index < PreloadedCount; ++Index )
    {
        Actor->EnqueueMessage( FMessage::Make( 10U, 0ULL ) );
    }

    /** INFO: First Tick is capped at MaxMessagesPerTick — no tick message inflates the count. */
    World.Tick( 0.0 );
    LUMEN_CHECK_EQ( Actor->GetReceivedTypes().size(), static_cast<USize>( FWorld::MaxMessagesPerTick ) );

    /** INFO: Second Tick drains the remaining 50. */
    World.Tick( 0.0 );
    LUMEN_CHECK_EQ( Actor->GetReceivedTypes().size(), static_cast<USize>( PreloadedCount ) );

    /** INFO: OnTick still fires every frame. */
    LUMEN_CHECK_EQ( Actor->GetTickCount(), 2U );
}

LUMEN_TEST( FWorld_Tick, ProcessesDiverseActorTypesInOnePass )
{
    FWorld World;

    const TSharedRef<FWorldTestActor> First = World.SpawnActor<FWorldTestActor>();
    const TSharedRef<FWorldAltActor> Alt    = World.SpawnActor<FWorldAltActor>();
    const TSharedRef<FWorldTestActor> Third = World.SpawnActor<FWorldTestActor>();

    World.Tick( 1.0 / 120.0 );

    /** INFO: No message was enqueued — Receive stays empty. */
    LUMEN_CHECK_EQ( First->GetReceivedTypes().size(), 0ULL );
    LUMEN_CHECK_EQ( Alt->GetReceivedTypes().size(), 0ULL );
    LUMEN_CHECK_EQ( Third->GetReceivedTypes().size(), 0ULL );

    /** INFO: OnTick fires on every actor, regardless of type. */
    LUMEN_CHECK_EQ( First->GetTickDelta(), 1.0 / 120.0 );
    LUMEN_CHECK_EQ( Third->GetTickDelta(), 1.0 / 120.0 );
}

} // namespace Lumen
