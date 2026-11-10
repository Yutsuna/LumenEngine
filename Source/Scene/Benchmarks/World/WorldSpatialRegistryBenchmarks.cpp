/**
 * @file WorldSpatialRegistryBenchmarks.cpp
 * @brief Benchmarks for the FSpatialRegistry singleton (update, publish, structural churn).
 */

#include <World/WorldSpatialRegistry.hpp>

#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Maths/Matrix.hpp>
#include <Templates/Vector.hpp>

namespace
{

/** @brief Background entity population shared by every benchmark here (power of two). */
constexpr Lumen::USize EntityCount = 1024U;

/** @brief Allocates Count unique IDs and registers them as spatial entities. */
void RegisterEntities ( Lumen::TVector<Lumen::ActorID> &OutEntityIds, const Lumen::USize Count )
{
    OutEntityIds.reserve( Count );

    for ( Lumen::USize EntityIndex = 0U; EntityIndex < Count; ++EntityIndex )
    {
        const Lumen::ActorID NewEntityId = Lumen::Internal::FActorRegistry::AllocateId();
        OutEntityIds.push_back( NewEntityId );

        Lumen::FSpatialRegistry::Get().RegisterSpatialEntity( NewEntityId );
    }
}

/** @brief Unregisters and releases every ID, leaving the singleton clean for the next benchmark. */
void ReleaseEntities ( const Lumen::TVector<Lumen::ActorID> &EntityIds )
{
    for ( const Lumen::ActorID EntityId : EntityIds )
    {
        Lumen::FSpatialRegistry::Get().UnregisterSpatialEntity( EntityId );
        Lumen::Internal::FActorRegistry::ReleaseId( EntityId );
    }
}

/** @brief Builds one identity transform per entity (untimed setup helper). */
[[nodiscard]] Lumen::TVector<Lumen::Maths::FMat4x4f> MakeTransforms ( const Lumen::USize Count )
{
    return Lumen::TVector<Lumen::Maths::FMat4x4f>( Count, Lumen::Maths::FMat4x4f::Identity() );
}

} // namespace

/**
 * Micro
 */

/**
 * Micro: UpdateTransform cycling over 1024 registered entities — the
 * per-entity hot path actors hit every frame (exclusive lock, IDToIndex
 * lookup, transform write, dirty-bit RMW).
 */
LUMEN_BENCHMARK( FSpatialRegistry, UpdateTransforms )
{
    Lumen::TVector<Lumen::ActorID> EntityIds;
    RegisterEntities( EntityIds, EntityCount );

    const Lumen::TVector<Lumen::Maths::FMat4x4f> Transforms = MakeTransforms( EntityCount );
    const Lumen::USize EntityIndexMask                      = EntityCount - 1U;
    Lumen::USize Cursor                                     = 0U;

    for ( auto _ : State )
    {
        Lumen::FSpatialRegistry::Get().UpdateTransform( EntityIds[Cursor], Transforms[Cursor] );
        Cursor = ( Cursor + 1U ) & EntityIndexMask;
    }

    Lumen::Benchmarks::DoNotOptimize( Cursor );

    ReleaseEntities( EntityIds );
}

/**
 * Micro: per-frame Publish on an idle scene — after the first (metadata)
 * pass, nothing is dirty, so this is the standing cost of the renderer
 * bridge when no entity moved.
 */
LUMEN_BENCHMARK( FSpatialRegistry, PublishIdle )
{
    Lumen::TVector<Lumen::ActorID> EntityIds;
    RegisterEntities( EntityIds, EntityCount );

    Lumen::FSpatialRegistry::Get().Publish();

    for ( auto _ : State )
    {
        Lumen::FSpatialRegistry::Get().Publish();
    }

    ReleaseEntities( EntityIds );
}

/**
 * Micro: Publish after every entity moved — re-dirtying 1024 transforms is
 * paused out; the measured span is the publish itself (pending-bit drain
 * into all three trackers, granular transform copy into the write buffer).
 */
LUMEN_BENCHMARK( FSpatialRegistry, PublishAllDirty )
{
    Lumen::TVector<Lumen::ActorID> EntityIds;
    RegisterEntities( EntityIds, EntityCount );

    const Lumen::TVector<Lumen::Maths::FMat4x4f> Transforms = MakeTransforms( EntityCount );
    State.SetItemsPerIteration( EntityCount );

    for ( auto _ : State )
    {
        State.PauseTiming();

        for ( Lumen::USize EntityIndex = 0U; EntityIndex < EntityCount; ++EntityIndex )
        {
            Lumen::FSpatialRegistry::Get().UpdateTransform( EntityIds[EntityIndex], Transforms[EntityIndex] );
        }

        State.ResumeTiming();

        Lumen::FSpatialRegistry::Get().Publish();
    }

    ReleaseEntities( EntityIds );
}

/**
 * Micro: register + unregister a probe entity against a populated registry —
 * the structural path (exclusive lock, swap-erase, per-component tracker
 * resize) paid by spawn/despawn storms.
 */
LUMEN_BENCHMARK( FSpatialRegistry, RegisterUnregisterEntity )
{
    Lumen::TVector<Lumen::ActorID> EntityIds;
    RegisterEntities( EntityIds, EntityCount );

    const Lumen::ActorID ProbeEntityId = Lumen::Internal::FActorRegistry::AllocateId();

    for ( auto _ : State )
    {
        Lumen::FSpatialRegistry::Get().RegisterSpatialEntity( ProbeEntityId );
        Lumen::FSpatialRegistry::Get().UnregisterSpatialEntity( ProbeEntityId );
    }

    Lumen::Internal::FActorRegistry::ReleaseId( ProbeEntityId );
    ReleaseEntities( EntityIds );
}
