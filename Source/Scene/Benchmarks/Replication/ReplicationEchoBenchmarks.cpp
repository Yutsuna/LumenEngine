/**
 * @file ReplicationEchoBenchmarks.cpp
 * @brief Full authority/replica roundtrip through an in-memory transport.
 */

#include <Replication/ReplicationSystem.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include "ReplicationBenchSupport.hpp"

/** @brief Steady-state snapshot roundtrip with two systems and two bounded byte rings. */
LUMEN_BENCHMARK_ARGS( FReplicationEcho, RoundTrip, 100, 1000, 10000 )
{
    const Lumen::Int64 Argument            = State.GetArgument();
    const Lumen::USize Count               = Argument > 0 ? static_cast<Lumen::USize>( Argument ) : 100ULL;
    const Lumen::FReplicationConfig Config = Lumen::BenchReplication::MakeConfig( Count );
    Lumen::BenchReplication::FMemoryTransport StateTransport( Config.MaxPayloadBytes );
    Lumen::BenchReplication::FNoopActorHandler AuthorityHandler;
    Lumen::BenchReplication::FNoopActorHandler ReplicaHandler;
    Lumen::FReplicationSystem Authority;
    Lumen::FReplicationSystem Replica;
    static_cast<void>( Authority.Initialize( Config, Lumen::EActorRole::Authority, StateTransport, AuthorityHandler ) );
    static_cast<void>( Replica.Initialize( Config, Lumen::EActorRole::Replica, StateTransport, ReplicaHandler ) );

    Lumen::TVector<Lumen::FReplicationActorSourceRecord> Source;
    Source.reserve( Count );
    Source.resize( Count );
    for ( Lumen::USize Index = 0ULL; Index < Count; ++Index )
    {
        Source[Index].LocalId = static_cast<Lumen::ActorID>( Index + 1ULL );
    }
    const Lumen::FReplicationSourceView View{ .Actors = Source, .FrameIndex = 1ULL };

    State.SetItemsPerIteration( Count );
    for ( auto _ : State )
    {
        const auto AuthorityResult = Authority.Tick( 1ULL, View );
        const auto ReplicaResult   = Replica.Tick( 1ULL, {} );
        Lumen::Benchmarks::DoNotOptimize( AuthorityResult );
        Lumen::Benchmarks::DoNotOptimize( ReplicaResult );
    }
}

/** @brief Idle endpoints: control polling and cursor maintenance with no dirty state. */
LUMEN_BENCHMARK( FReplicationEcho, Idle )
{
    const Lumen::FReplicationConfig Config = Lumen::BenchReplication::MakeConfig( 16ULL );
    Lumen::BenchReplication::FMemoryTransport StateTransport( Config.MaxPayloadBytes );
    Lumen::BenchReplication::FNoopActorHandler Handler;
    Lumen::FReplicationSystem System;
    static_cast<void>( System.Initialize( Config, Lumen::EActorRole::Authority, StateTransport, Handler ) );

    State.SetItemsPerIteration( 1ULL );
    for ( auto _ : State )
    {
        const auto Result = System.Tick( 1ULL, {} );
        Lumen::Benchmarks::DoNotOptimize( Result );
    }
}
