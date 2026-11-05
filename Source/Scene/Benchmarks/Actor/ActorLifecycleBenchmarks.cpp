/// @file ActorLifecycleBenchmarks.cpp
/// @brief Publication and teardown cost of an actor.
///
/// Registration is not on the per-message path, but it determines how expensive actor creation is for a scene. The
/// benchmark creates a fresh actor, publishes it, reads its immutable ID and shuts it down in a fixed batch so the
/// registry and generation-history costs remain visible without growing the benchmark loop by millions of actors.

#include <Benchmarks/Benchmarks.hpp>

#include <Actor/Actor.hpp>

#include "ActorBenchmarkHelpers.hpp"

namespace Lumen
{

LUMEN_MACRO_BENCHMARK( ActorLifecycle, RegisterShutdown )
{
    constexpr UInt32 Batch = 1024U;
    State.SetItemsPerIteration( Batch );

    for ( auto _ : State )
    {
        for ( UInt32 Index = 0U; Index < Batch; ++Index )
        {
            FNoOpBenchmarkActor Actor;
            const EActorRegistrationResult Registration = Actor.Initialize();
            Benchmarks::DoNotOptimize( Registration );
            Benchmarks::DoNotOptimize( Actor.GetId() );

            Actor.Shutdown();
        }
    }
}

} // namespace Lumen
