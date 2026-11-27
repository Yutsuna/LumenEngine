/**
 * @file WorldCapacityTests.cpp
 * @brief Tests structured spawn/destroy/queue failures and bounded outboxes.
 */

#include <Actor/ActorLifecycle.hpp>
#include <World/WorldTypes.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>
#include <Tests/UnitTest.hpp>
#include <World/World.hpp>

namespace Lumen
{

namespace
{

    /** @class FNoopActor
     * @brief Minimal actor used as a message target.
     */
    class FNoopActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FNoopActor );

    public:

        explicit FNoopActor ( const ActorID InId ) noexcept : AActor( InId )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
        }
    };

    /** @class FOutboxActor
     * @brief Emits twice in one task to exercise the bounded outbox.
     */
    class FOutboxActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FOutboxActor );

    public:

        FOutboxActor ( const ActorID InId, const ActorID InTarget, TAtomic<UInt32> *InResults ) noexcept
            : AActor( InId ), Target( InTarget ), Results( InResults )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
        }

        void OnTick ( const Float64 ) noexcept override
        {
            const FMessage Message                        = FMessage::Make( 83U, GetId() );
            const TExpected<void, EActorSendError> First  = TrySendMessage( Target, Message );
            const TExpected<void, EActorSendError> Second = TrySendMessage( Target, Message );

            Results[0].store( First ? 0U : static_cast<UInt32>( First.Error() ) + 1U, std::memory_order_release );
            Results[1].store( Second ? 0U : static_cast<UInt32>( Second.Error() ) + 1U, std::memory_order_release );
        }

    private:

        ActorID Target           = InvalidActorID;
        TAtomic<UInt32> *Results = nullptr;
    };

} // namespace

LUMEN_TEST( FWorld_Capacity, SpawnBeyondMaxActorsIsStructured )
{
    FWorldDesc Desc;
    Desc.WorkerCount = 1U;
    Desc.MaxActors   = 1ULL;

    FWorld World( Desc );

    const TExpected<TSharedRef<FNoopActor>, EActorSpawnError> First = World.TrySpawnActor<FNoopActor>();
    LUMEN_CHECK_HAS_VALUE( First );

    const TExpected<TSharedRef<FNoopActor>, EActorSpawnError> Second = World.TrySpawnActor<FNoopActor>();
    LUMEN_CHECK_HAS_ERROR( Second );
    LUMEN_CHECK_EQ( Second.Error(), EActorSpawnError::WorldCapacityExceeded );
}

LUMEN_TEST( FWorld_Capacity, DestroyReportsInvalidAndUnknownIds )
{
    FWorld World;

    const TExpected<void, EActorDestroyError> Invalid = World.TryDestroyActor( InvalidActorID );
    LUMEN_CHECK_HAS_ERROR( Invalid );
    LUMEN_CHECK_EQ( Invalid.Error(), EActorDestroyError::InvalidActor );

    constexpr ActorID UnknownId                       = 0x0000'0000'0200'0000ULL;
    const TExpected<void, EActorDestroyError> Unknown = World.TryDestroyActor( UnknownId );
    LUMEN_CHECK_HAS_ERROR( Unknown );
    LUMEN_CHECK_EQ( Unknown.Error(), EActorDestroyError::UnknownActor );
}

LUMEN_TEST( FWorld_Capacity, DeferredQueueFullIsReportedAndPreviousCommandSurvives )
{
    FWorldDesc Desc;
    Desc.WorkerCount             = 1U;
    Desc.DeferredCommandCapacity = 1ULL;

    FWorld World( Desc );

    const TExpected<FActorRef, EActorSpawnError> First = World.TrySpawnActorDeferred<FNoopActor>();
    LUMEN_CHECK_HAS_VALUE( First );

    const TExpected<FActorRef, EActorSpawnError> Second = World.TrySpawnActorDeferred<FNoopActor>();
    LUMEN_CHECK_HAS_ERROR( Second );
    LUMEN_CHECK_EQ( Second.Error(), EActorSpawnError::DeferredQueueFull );

    const TExpected<FWorldTickResult, EWorldTickError> Tick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( Tick );

    const UInt64 Before = Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::ActorRetiring );
    const TExpected<void, EActorSendError> Routed = First->TryEnqueueMessage( FMessage::Make( 84U, 0ULL ) );
    LUMEN_CHECK_HAS_VALUE( Routed );
    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::ActorRetiring ), Before );
}

LUMEN_TEST( FWorld_Capacity, OutboxFullRecordsStructuredDeadLetter )
{
    FWorldDesc Desc;
    Desc.WorkerCount         = 1U;
    Desc.ActorOutboxCapacity = 1ULL;
    Desc.EmissionCapacity    = 4ULL;

    FWorld World( Desc );
    auto Target = World.SpawnActor<FNoopActor>();

    TAtomic<UInt32> Results[2] = { { 0U }, { 0U } };
    auto Sender                = World.SpawnActor<FOutboxActor>( Target->GetId(), Results );

    const UInt64 Before = Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::OutboxFull );

    const TExpected<FWorldTickResult, EWorldTickError> Tick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( Tick );

    LUMEN_CHECK_EQ( Results[0].load( std::memory_order_acquire ), 0U );
    LUMEN_CHECK_EQ( Results[1].load( std::memory_order_acquire ), static_cast<UInt32>( EActorSendError::OutboxFull ) + 1U );
    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::OutboxFull ) - Before, 1ULL );
    LUMEN_CHECK_EQ( Tick->RoutedMessageCount, 1ULL );
}

LUMEN_TEST( FWorld_Capacity, PendingSpawnsAreRetiredWhenWorldIsDestroyed )
{
    ActorID PendingId = InvalidActorID;

    {
        FWorld World;
        const TExpected<FActorRef, EActorSpawnError> Pending = World.TrySpawnActorDeferred<FNoopActor>();
        LUMEN_CHECK_HAS_VALUE( Pending );
        PendingId = Pending->GetId();
        /** INFO: No tick — the spawn command is still queued when the world dies. */
    }

    const UInt64 Before = Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::StaleActor );
    const FActorRef Ref( PendingId );
    const TExpected<void, EActorSendError> Sent = Ref.TryEnqueueMessage( FMessage::Make( 85U, 0ULL ) );

    LUMEN_CHECK_HAS_ERROR( Sent );
    LUMEN_CHECK_EQ( Sent.Error(), EActorSendError::StaleActor );
    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::StaleActor ) - Before, 1ULL );
}

} // namespace Lumen
