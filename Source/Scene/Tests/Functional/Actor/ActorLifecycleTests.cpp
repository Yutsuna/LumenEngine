/**
 * @file ActorLifecycleTests.cpp
 * @brief Tests logical lifecycle, hook idempotency and safe retirement.
 */

#include <Actor/ActorLifecycle.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>
#include <Memory/SharedPtr.hpp>
#include <Tests/UnitTest.hpp>
#include <World/World.hpp>

namespace Lumen
{

namespace
{

    /** @struct FLifecycleProbe
     * @brief Atomic observations collected by a lifecycle actor.
     */
    struct FLifecycleProbe
    {
        TAtomic<UInt32> BeginCount    = { 0U };
        TAtomic<UInt32> EndCount      = { 0U };
        TAtomic<UInt32> ReceiveCount  = { 0U };
        TAtomic<UInt32> TickCount     = { 0U };
        TAtomic<UInt32> DestroyResult = { 0U };
        TAtomic<UInt32> SelfSendError = { 0U };
        TAtomic<Bool> bWasInParallel  = { false };
    };

    /** @class FLifecycleActor
     * @brief Counts hooks and optionally performs control operations from a task.
     */
    class FLifecycleActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FLifecycleActor );

    public:

        FLifecycleActor (
            const ActorID InId, FLifecycleProbe *InProbe, FWorld *InWorld, Bool bInDestroyOnTick, Bool bInSendOnTick ) noexcept
            : AActor( InId ), Probe( InProbe ), World( InWorld ), bDestroyOnTick( bInDestroyOnTick ), bSendOnTick( bInSendOnTick )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
            Probe->ReceiveCount.fetch_add( 1U, std::memory_order_relaxed );
        }

        void OnBeginPlay () noexcept override
        {
            Probe->BeginCount.fetch_add( 1U, std::memory_order_relaxed );
        }

        void OnEndPlay () noexcept override
        {
            Probe->EndCount.fetch_add( 1U, std::memory_order_relaxed );
        }

        void OnTick ( const Float64 ) noexcept override
        {
            Probe->TickCount.fetch_add( 1U, std::memory_order_relaxed );
            Probe->bWasInParallel.store( World->IsInParallelPhase(), std::memory_order_release );

            if ( bDestroyOnTick )
            {
                TExpected<void, EActorDestroyError> Result = World->TryDestroyActor( GetId() );
                Probe->DestroyResult.store( Result ? 1U : static_cast<UInt32>( Result.Error() ) + 2U, std::memory_order_release );
            }

            if ( bSendOnTick )
            {
                TExpected<void, EActorSendError> Result = TrySendMessage( GetId(), FMessage::Make( 75U, GetId() ) );
                Probe->SelfSendError.store( Result ? 0U : static_cast<UInt32>( Result.Error() ) + 1U, std::memory_order_release );
            }
        }

    private:

        FLifecycleProbe *Probe = nullptr;
        FWorld *World          = nullptr;
        Bool bDestroyOnTick    = false;
        Bool bSendOnTick       = false;
    };

} // namespace

LUMEN_TEST( FActor_Lifecycle, HooksRunExactlyOnce )
{
    FWorld World;
    FLifecycleProbe Probe;

    auto Actor = World.SpawnActor<FLifecycleActor>( &Probe, &World, false, false );

    LUMEN_CHECK_EQ( Probe.BeginCount.load( std::memory_order_acquire ), 1U );
    LUMEN_CHECK_EQ( Probe.EndCount.load( std::memory_order_acquire ), 0U );
    LUMEN_CHECK_EQ( Actor->GetLifecycle(), EActorLifecycle::Active );

    const TExpected<void, EActorDestroyError> Destroyed = World.TryDestroyActor( Actor->GetId() );
    LUMEN_CHECK_HAS_VALUE( Destroyed );

    LUMEN_CHECK_EQ( Probe.EndCount.load( std::memory_order_acquire ), 1U );
    LUMEN_CHECK_EQ( Actor->GetLifecycle(), EActorLifecycle::Retired );

    const TExpected<void, EActorDestroyError> Again = World.TryDestroyActor( Actor->GetId() );
    LUMEN_CHECK_HAS_ERROR( Again );
    LUMEN_CHECK_EQ( Again.Error(), EActorDestroyError::UnknownActor );
    LUMEN_CHECK_EQ( Probe.EndCount.load( std::memory_order_acquire ), 1U );
}

LUMEN_TEST( FActor_Lifecycle, ExternalStrongRefKeepsOnlyStorageAlive )
{
    FLifecycleProbe Probe;
    ActorID RetiredId = InvalidActorID;
    TSharedPtr<AActor> Kept;

    {
        FWorld World;
        auto Actor = World.SpawnActor<FLifecycleActor>( &Probe, &World, false, false );
        RetiredId  = Actor->GetId();
        Kept       = TSharedPtr<AActor>( Actor );

        const TExpected<void, EActorDestroyError> Destroyed = World.TryDestroyActor( RetiredId );
        LUMEN_CHECK_HAS_VALUE( Destroyed );
        LUMEN_CHECK_EQ( Kept->GetLifecycle(), EActorLifecycle::Retired );
    }

    /** INFO: The world is gone but the external strong ref still owns storage. */
    LUMEN_CHECK_TRUE( Kept.IsValid() );
    LUMEN_CHECK_EQ( Kept->GetLifecycle(), EActorLifecycle::Retired );
    LUMEN_CHECK_EQ( Probe.EndCount.load( std::memory_order_acquire ), 1U );

    const UInt64 Before = Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::StaleActor );
    const FActorRef Ref( RetiredId );
    const TExpected<void, EActorSendError> Sent = Ref.TryEnqueueMessage( FMessage::Make( 76U, 0ULL ) );

    LUMEN_CHECK_HAS_ERROR( Sent );
    LUMEN_CHECK_EQ( Sent.Error(), EActorSendError::StaleActor );
    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::StaleActor ) - Before, 1ULL );
    LUMEN_CHECK_EQ( Probe.ReceiveCount.load( std::memory_order_acquire ), 0U );
}

LUMEN_TEST( FActor_Lifecycle, DeferredSpawnKeepsRoutingClosedUntilActivation )
{
    FWorld World;
    FLifecycleProbe Probe;

    const TExpected<FActorRef, EActorSpawnError> Spawned =
        World.TrySpawnActorDeferred<FLifecycleActor>( &Probe, &World, false, false );
    LUMEN_CHECK_HAS_VALUE( Spawned );

    const FActorRef Ref = *Spawned;
    LUMEN_CHECK_EQ( Probe.BeginCount.load( std::memory_order_acquire ), 0U );

    const UInt64 Before = Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::ActorRetiring );
    const TExpected<void, EActorSendError> Blocked = Ref.TryEnqueueMessage( FMessage::Make( 77U, 0ULL ) );
    LUMEN_CHECK_HAS_ERROR( Blocked );
    LUMEN_CHECK_EQ( Blocked.Error(), EActorSendError::ActorRetiring );
    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::ActorRetiring ) - Before, 1ULL );

    const TExpected<FWorldTickResult, EWorldTickError> Tick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( Tick );
    LUMEN_CHECK_EQ( Probe.BeginCount.load( std::memory_order_acquire ), 1U );

    const UInt64 AfterActivation = Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::ActorRetiring );
    const TExpected<void, EActorSendError> Routed = Ref.TryEnqueueMessage( FMessage::Make( 78U, 0ULL ) );
    LUMEN_CHECK_HAS_VALUE( Routed );
    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::ActorRetiring ), AfterActivation );
}

LUMEN_TEST( FActor_Lifecycle, DestroyFromTaskIsDeferredToNextTick )
{
    FWorld World;
    FLifecycleProbe Probe;

    auto Actor = World.SpawnActor<FLifecycleActor>( &Probe, &World, true, true );

    const TExpected<FWorldTickResult, EWorldTickError> FirstTick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( FirstTick );
    LUMEN_CHECK_EQ( Probe.DestroyResult.load( std::memory_order_acquire ), 1U );
    LUMEN_CHECK_EQ( Probe.SelfSendError.load( std::memory_order_acquire ), 0U );
    LUMEN_CHECK_TRUE( Probe.bWasInParallel.load( std::memory_order_acquire ) );

    /** INFO: Destroy requested inside the phase applies only at the next serial commit. */
    LUMEN_CHECK_EQ( Actor->GetLifecycle(), EActorLifecycle::Active );
    LUMEN_CHECK_EQ( Probe.EndCount.load( std::memory_order_acquire ), 0U );

    const TExpected<FWorldTickResult, EWorldTickError> SecondTick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( SecondTick );
    LUMEN_CHECK_EQ( Actor->GetLifecycle(), EActorLifecycle::Retired );
    LUMEN_CHECK_EQ( Probe.EndCount.load( std::memory_order_acquire ), 1U );
    LUMEN_CHECK_EQ( Probe.TickCount.load( std::memory_order_acquire ), 1U );
}

LUMEN_TEST( FActor_Lifecycle, IDChurnNeverReusesALiveGeneration )
{
    FWorld World;
    FLifecycleProbe Probe;

    constexpr UInt32 CycleCount = 100000U;
    ActorID PreviousId          = InvalidActorID;
    UInt32 ReusedSlotCount      = 0U;
    UInt32 SampledStaleCount    = 0U;

    for ( UInt32 CycleIndex = 0U; CycleIndex < CycleCount; ++CycleIndex )
    {
        const TExpected<TSharedRef<FLifecycleActor>, EActorSpawnError> Spawned =
            World.TrySpawnActor<FLifecycleActor>( &Probe, &World, false, false );
        LUMEN_CHECK_HAS_VALUE( Spawned );

        const ActorID CurrentId = ( *Spawned )->GetId();
        LUMEN_CHECK_NOT_EQ( CurrentId, InvalidActorID );
        LUMEN_CHECK_NOT_EQ( CurrentId, PreviousId );
        LUMEN_CHECK_EQ( ( *Spawned )->GetLifecycle(), EActorLifecycle::Active );

        if ( PreviousId != InvalidActorID and ActorIdSlotIndex( CurrentId ) == ActorIdSlotIndex( PreviousId ) )
        {
            ++ReusedSlotCount;
        }

        const TExpected<void, EActorDestroyError> Destroyed = World.TryDestroyActor( CurrentId );
        LUMEN_CHECK_HAS_VALUE( Destroyed );
        LUMEN_CHECK_EQ( ( *Spawned )->GetLifecycle(), EActorLifecycle::Retired );

        if ( CycleIndex % 10000U == 0U )
        {
            const TExpected<void, EActorSendError> Sent = FActorRef( CurrentId ).TryEnqueueMessage( FMessage::Make( 89U, 0ULL ) );
            LUMEN_CHECK_HAS_ERROR( Sent );
            LUMEN_CHECK_EQ( Sent.Error(), EActorSendError::StaleActor );
            ++SampledStaleCount;
        }

        PreviousId = CurrentId;
    }

    /** INFO: Every cycle recycles the slot with a fresh generation — no stale route survives. */
    LUMEN_CHECK_EQ( ReusedSlotCount, CycleCount - 1U );
    LUMEN_CHECK_EQ( SampledStaleCount, CycleCount / 10000U );
    LUMEN_CHECK_EQ( Probe.BeginCount.load( std::memory_order_acquire ), CycleCount );
    LUMEN_CHECK_EQ( Probe.EndCount.load( std::memory_order_acquire ), CycleCount );
}

LUMEN_TEST( FActor_Lifecycle, WorldDestructionEndsPlayExactlyOnce )
{
    FLifecycleProbe Probe;
    TSharedPtr<AActor> Kept;

    {
        FWorld World;
        auto Actor = World.SpawnActor<FLifecycleActor>( &Probe, &World, false, false );
        Kept       = TSharedPtr<AActor>( Actor );

        LUMEN_CHECK_EQ( Probe.BeginCount.load( std::memory_order_acquire ), 1U );
        LUMEN_CHECK_EQ( Probe.EndCount.load( std::memory_order_acquire ), 0U );
    }

    LUMEN_CHECK_EQ( Kept->GetLifecycle(), EActorLifecycle::Retired );
    LUMEN_CHECK_EQ( Probe.EndCount.load( std::memory_order_acquire ), 1U );
}

} // namespace Lumen
