/**
 * @file WorldPhaseTickedTests.cpp
 * @brief Tests the phased tick mechanism in the world.
 */

#include <World/World.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /**
     * @class FSpawnerActor
     * @brief Spawns a new FSpawnerActor when triggered.
     */
    class FSpawnerActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FSpawnerActor );

    public:

        FSpawnerActor ( ActorID InId, FWorld *InWorld ) noexcept : AActor( InId ), World( InWorld )
        {
            /* */
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            if ( InMessage.Type == TriggerType )
            {
                SpawnedRef = World->SpawnActorDeferred<FSpawnerActor>( World );
            }
        }

        static constexpr UInt32 TriggerType = 42U;

        FWorld *World = nullptr;
        FActorRef SpawnedRef;
    };

    /**
     * @class FSelfDestructActor
     * @brief Destroys itself when triggered.
     */
    class FSelfDestructActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FSelfDestructActor );

    public:

        FSelfDestructActor ( ActorID InId, FWorld *InWorld ) noexcept : AActor( InId ), World( InWorld )
        {
            /*  */
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            if ( InMessage.Type == TriggerType and not bAlreadyTriggered )
            {
                bAlreadyTriggered = true;
                World->DestroyActor( GetId() );
            }
            else if ( InMessage.Type == ProbeType )
            {
                ProbeCount.fetch_add( 1U, std::memory_order_relaxed );
            }
        }

        [[nodiscard]] UInt32 GetProbeCount () const noexcept
        {
            return ProbeCount.load( std::memory_order_relaxed );
        }

        static constexpr UInt32 TriggerType = 43U;
        static constexpr UInt32 ProbeType   = 44U;

        FWorld *World = nullptr;

    private:

        Bool bAlreadyTriggered             = false;
        mutable TAtomic<UInt32> ProbeCount = { 0U };
    };

} // namespace

LUMEN_TEST( FWorld_PhasedTick, SpawnFromReceiveIsDeferred )
{
    FWorld World;
    auto Spawner = World.SpawnActor<FSpawnerActor>( &World );

    Spawner->EnqueueMessage( FMessage::Make( FSpawnerActor::TriggerType, 0ULL ) );
    World.Tick( 0.0 );

    LUMEN_CHECK_TRUE( Spawner->SpawnedRef.IsValid() );
}

LUMEN_TEST( FWorld_PhasedTick, DestroyFromReceiveIsDeferred )
{
    FWorld World;
    auto Victim = World.SpawnActor<FSelfDestructActor>( &World );

    const FActorRef Target = Victim->GetRef();

    /**
     * INFO: Tick 1 — Receive fires DestroyActor. Because Receive runs inside the
     *       parallel phase, DestroyActor defers its effect to the next Tick.
     *       So the actor is still removed from ActiveActors only at the start
     *       of Tick 2 — never during Tick 1.
     */
    Victim->EnqueueMessage( FMessage::Make( FSelfDestructActor::TriggerType, 0ULL ) );
    World.Tick( 0.0 );

    /** Tick 1 must not have triggered the destroy yet. */
    LUMEN_CHECK_EQ( Victim->GetProbeCount(), 0U );

    /**
     * INFO: Tick 2 — ApplyDeferredCommands runs DestroyActorImpl BEFORE the
     *       parallel phase, so the actor is out of ActiveActors and is no
     *       longer ticked. The probe below is enqueued AFTER Tick 2 started
     *       applying, which is fine — it lands in a mailbox nobody drains.
     */
    Target.EnqueueMessage( FMessage::Make( FSelfDestructActor::ProbeType, 0ULL ) );
    World.Tick( 0.0 );

    /**
     * INFO: The probe never fires because the actor was removed from
     *       ActiveActors by ApplyDeferredCommands before the parallel phase
     *       of Tick 2 — RunFrame is never called on it again.
     *       Victim's TSharedRef keeps the object alive, so ~AActor (and
     *       therefore the registry slot retirement) has not happened; the
     *       contract being tested is "no longer ticked", not "freed".
     */
    LUMEN_CHECK_EQ( Victim->GetProbeCount(), 0U );
}

/**
 * Destroy
 */

LUMEN_TEST( FWorld_DeferredSpawn, DestroyedWorldReleasesUnappliedIds )
{
    ActorID DeferredId = InvalidActorID;

    {
        FWorld World;
        const FActorRef Ref = World.SpawnActorDeferred<FSpawnerActor>( &World );
        DeferredId          = Ref.GetId();
        LUMEN_CHECK_NOT_EQ( DeferredId, InvalidActorID );
        /** INFO: No Tick — the command stays in DeferredCommands. */
    }

    /** INFO: ~FWorld runs before the free-list state is observed. Before the
     *        fix the slot stayed owned by a never-constructed actor, leaking
     *        one registry slot per abandoned deferred spawn. */
    Bool bFound = false;
    TVector<ActorID> Allocated;

    for ( UInt32 Probe = 0U; Probe < 256U and not bFound; ++Probe )
    {
        const ActorID Probed = Internal::FActorRegistry::AllocateId();
        LUMEN_CHECK_NOT_EQ( Probed, InvalidActorID );

        if ( ActorIdSlotIndex( Probed ) == ActorIdSlotIndex( DeferredId ) )
        {
            /** INFO: Generation was bumped on release — a fresh occupant. */
            LUMEN_CHECK_NOT_EQ( Probed, DeferredId );
            bFound = true;
        }
        else
        {
            Allocated.push_back( Probed );
        }
    }

    LUMEN_CHECK_TRUE( bFound );

    /** INFO: Don't leak probe allocations into following suites. */
    for ( const ActorID Probed : Allocated )
    {
        Internal::FActorRegistry::ReleaseId( Probed );
    }
}

} // namespace Lumen
