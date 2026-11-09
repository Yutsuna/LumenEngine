/**
 * @file WorldBenchmarks.cpp
 * @brief Benchmarks for FWorld level-build and full-frame tick costs.
 */

#include <World/World.hpp>
#include <World/WorldSpatialRegistry.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Maths/Matrix.hpp>
#include <Maths/Vec.hpp>

#include <Memory/SharedPtr.hpp>
#include <Messages/EngineMessageTypes.hpp>
#include <Templates/Vector.hpp>

namespace
{

/** @brief Delta time used for every simulated frame. */
constexpr Lumen::Float64 FrameDeltaTime = 0.016;

/** @brief Actor counts of the scene sizes the tick macro benchmarks cover. */
constexpr Lumen::USize SmallSceneActorCount  = 100U;
constexpr Lumen::USize MediumSceneActorCount = 1000U;
constexpr Lumen::USize LargeSceneActorCount  = 10000U;

/** @brief Upper bound of the spawn range benchmark. */
constexpr Lumen::USize MaxSpawnActors = 4096U;

/**
 * @class FBenchNullActor
 * @brief Actor with no behaviour — isolates the pure spawn/teardown path.
 */
class FBenchNullActor final : public Lumen::AActor
{
    LUMEN_ACTOR_BODY ( FBenchNullActor );

public:

    explicit FBenchNullActor ( const Lumen::ActorID InId ) noexcept : Lumen::AActor( InId )
    {
        /**
         *
         */
    }

    void Receive ( const Lumen::FMessage & ) noexcept override
    {
        /**
         *
         */
    }
};

/**
 * @class FBenchSceneActor
 * @brief Actor moving every frame: Receive counts, OnTick pushes the new
 *        transform into the spatial registry — the per-entity work of a
 *        real scene frame, including the world's Publish.
 */
class FBenchSceneActor final : public Lumen::AActor
{
    LUMEN_ACTOR_BODY ( FBenchSceneActor );

public:

    explicit FBenchSceneActor ( const Lumen::ActorID InId ) noexcept : Lumen::AActor( InId )
    {
        Lumen::FSpatialRegistry::Get().RegisterSpatialEntity( InId );
    }

    ~FBenchSceneActor () noexcept override
    {
        /** INFO: Covers both world teardown and DestroyActor (idempotent). */
        Lumen::FSpatialRegistry::Get().UnregisterSpatialEntity( GetId() );
    }

    void Receive ( const Lumen::FMessage &InMessage ) noexcept override
    {
        ProcessedMessageCount += InMessage.Type;
    }

    void OnTick ( const Lumen::Float64 InDeltaTime ) noexcept override
    {
        const Lumen::Float32 SweepOffset =
            static_cast<Lumen::Float32>( TickIndex ) * 0.01F + static_cast<Lumen::Float32>( InDeltaTime );

        Transform =
            Lumen::Maths::FMatrix4x4f::Translate( Lumen::Maths::FVec3f{ SweepOffset, SweepOffset, SweepOffset } ) * Transform;

        Lumen::FSpatialRegistry::Get().UpdateTransform( GetId(), Transform );
        ++TickIndex;
    }

private:

    Lumen::Maths::FMatrix4x4f Transform = Lumen::Maths::FMatrix4x4f::Identity();
    Lumen::UInt32 TickIndex             = 0U;
    Lumen::UInt32 ProcessedMessageCount = 0U;
};

/** @brief Spawns Count null actors into the world (untimed setup helper). */
void PopulateWorldWithNullActors ( Lumen::FWorld &World, const Lumen::USize ActorCount )
{
    for ( Lumen::USize ActorIndex = 0U; ActorIndex < ActorCount; ++ActorIndex )
    {
        Lumen::TSharedRef<FBenchNullActor> Spawned = World.SpawnActor<FBenchNullActor>();
        Lumen::Benchmarks::DoNotOptimize( Spawned );
    }
}

/** @brief Spawns Count scene actors into the world (untimed setup helper). */
void PopulateWorldWithSceneActors ( Lumen::FWorld &World, const Lumen::USize ActorCount )
{
    for ( Lumen::USize ActorIndex = 0U; ActorIndex < ActorCount; ++ActorIndex )
    {
        Lumen::TSharedRef<FBenchSceneActor> Spawned = World.SpawnActor<FBenchSceneActor>();
        Lumen::Benchmarks::DoNotOptimize( Spawned );
    }
}

} // namespace

/**
 * Micro
 */

/**
 * Micro: SpawnActor of Argument actors into a fresh world; world construction
 * and destruction are paused out, so the sample is the level-build path
 * itself (ID allocation, actor construction, world container inserts).
 */
LUMEN_BENCHMARK_RANGE( FWorld, SpawnActors, 1, MaxSpawnActors )
{
    const Lumen::USize ActorCount = static_cast<Lumen::USize>( State.GetArgument() );
    State.SetItemsPerIteration( ActorCount );

    for ( auto _ : State )
    {
        State.PauseTiming();

        Lumen::FWorld World;

        State.ResumeTiming();

        PopulateWorldWithNullActors( World, ActorCount );

        State.PauseTiming();
    }
}

/**
 * Macro
 */

/**
 * Macro: one full FWorld::Tick on a 100-actor scene — deferred-command sweep,
 * parallel RunFrame over every actor (transform update + registry publish).
 */
LUMEN_MACRO_BENCHMARK( FWorld, TickFrame100Actors )
{
    Lumen::FWorld World;
    PopulateWorldWithSceneActors( World, SmallSceneActorCount );

    /** INFO: Untimed warm-up — first Publish after registration does a full
     *        metadata copy, and the parallel executor needs a tick to spin up. */
    World.Tick( FrameDeltaTime );

    for ( auto _ : State )
    {
        World.Tick( FrameDeltaTime );
    }
}

/**
 * Macro: same full-frame tick as TickFrame100Actors, on a 1000-actor scene.
 */
LUMEN_MACRO_BENCHMARK( FWorld, TickFrame1kActors )
{
    Lumen::FWorld World;
    PopulateWorldWithSceneActors( World, MediumSceneActorCount );

    /** INFO: Untimed warm-up — see TickFrame100Actors. */
    World.Tick( FrameDeltaTime );

    for ( auto _ : State )
    {
        World.Tick( FrameDeltaTime );
    }
}

/**
 * Macro: same full-frame tick as TickFrame100Actors, on a 10000-actor scene —
 * measures how the parallel phase and the spatial publish scale.
 */
LUMEN_MACRO_BENCHMARK( FWorld, TickFrame10kActors )
{
    Lumen::FWorld World;
    PopulateWorldWithSceneActors( World, LargeSceneActorCount );

    /** INFO: Untimed warm-up — see TickFrame100Actors. */
    World.Tick( FrameDeltaTime );

    for ( auto _ : State )
    {
        World.Tick( FrameDeltaTime );
    }
}

/**
 * Macro: a 1000-actor frame where every actor also receives one inbound
 * message (Broadcast + Tick) — exercises the mailbox fence, message
 * processing and the MaxMessagesPerTick path inside the world tick.
 */
LUMEN_MACRO_BENCHMARK( FWorld, TickFrame1kActorsWithMail )
{
    Lumen::FWorld World;
    PopulateWorldWithSceneActors( World, MediumSceneActorCount );

    const Lumen::FMessage Message = Lumen::FMessage::Make( Lumen::EEngineMessage::Tick, Lumen::InvalidActorID,
                                                           Lumen::FTickPayload{ .DeltaTime = FrameDeltaTime } );

    /** INFO: Untimed warm-up — see TickFrame100Actors. */
    World.Tick( FrameDeltaTime );

    for ( auto _ : State )
    {
        World.Broadcast( Message );
        World.Tick( FrameDeltaTime );
    }
}
