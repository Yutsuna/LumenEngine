/**
 * @file ReplicationReplayTests.cpp
 * @brief Literal-seed replay of replicated state with canonical digest comparison.
 */

#include <Replication/ReplicationSystem.hpp>

#include <Tests/UnitTest.hpp>

#include "ReplicationTestSupport.hpp"

namespace Lumen
{
namespace
{

    constexpr UInt64 Seeds[]        = { 17ULL, 0x5EEDULL, 987654321ULL };
    constexpr UInt32 WorkerCounts[] = { 1U, 2U, 4U };
    constexpr UInt32 Runs           = 32U;
    constexpr UInt64 TickCount      = 24ULL;
    constexpr USize ActorCount      = 16ULL;

    UInt64 NextRandom ( UInt64 &InState ) noexcept
    {
        InState = ( InState * 6364136223846793005ULL ) + 1442695040888963407ULL;
        return ( InState >> 33ULL ) & 0xFFFFULL;
    }

    UInt64 RunScenario ( const UInt64 InSeed ) noexcept
    {
        using namespace ReplicationTests;
        const FReplicationConfig Config = MakeConfig( ActorCount );
        FMemoryTransport StateTransport( Config.MaxPayloadBytes );
        FRecordingHandler AuthorityHandler;
        FRecordingHandler ReplicaHandler;
        FReplicationSystem Authority;
        FReplicationSystem Replica;
        if ( not Authority.Initialize( Config, EActorRole::Authority, StateTransport, AuthorityHandler ) )
        {
            return 0ULL;
        }
        if ( not Replica.Initialize( Config, EActorRole::Replica, StateTransport, ReplicaHandler ) )
        {
            return 0ULL;
        }

        TVector<FReplicationActorSourceRecord> Source;
        Source.reserve( ActorCount );
        Source.resize( ActorCount );
        for ( USize Index = 0ULL; Index < ActorCount; ++Index )
        {
            Source[Index].LocalId = static_cast<ActorID>( Index + 1ULL );
        }

        UInt64 Random = InSeed;
        for ( UInt64 Frame = 1ULL; Frame <= TickCount; ++Frame )
        {
            for ( FReplicationActorSourceRecord &Record : Source )
            {
                const Float32 Offset  = static_cast<Float32>( NextRandom( Random ) % 97ULL );
                Record.LocalTransform = Maths::FMat4x4f::Translate( Maths::FVec3f{ Offset, 0.0F, 0.0F } );
            }
            if ( Frame % 4ULL == 0ULL )
            {
                const FPersistentActorId Target{ ( NextRandom( Random ) % ActorCount ) + 1ULL };
                static_cast<void>( Authority.SetVisibility( Target, ( Frame % 8ULL ) == 0ULL ) );
            }
            const FReplicationSourceView View{ .Actors = Source, .FrameIndex = Frame };
            static_cast<void>( Authority.Tick( Frame, View ) );
            static_cast<void>( Replica.Tick( Frame, {} ) );
        }
        return ReplicaHandler.GetDigest();
    }

} // namespace

LUMEN_INTEGRATION_TEST( FReplicationReplay, CanonicalDigestIsSeedStableAcrossRuns )
{
    for ( const UInt64 Seed : Seeds )
    {
        UInt64 Canonical = 0ULL;
        for ( UInt32 Run = 0U; Run < Runs; ++Run )
        {
            for ( const UInt32 Workers : WorkerCounts )
            {
                static_cast<void>( Workers );
                const UInt64 Digest = RunScenario( Seed );
                LUMEN_CHECK_NOT_EQ( Digest, 0ULL );
                if ( Canonical == 0ULL )
                {
                    Canonical = Digest;
                }
                LUMEN_CHECK_EQ( Digest, Canonical );
            }
        }
    }
}

LUMEN_INTEGRATION_TEST( FReplicationReplay, DifferentSeedsProduceDifferentDigests )
{
    const UInt64 First  = RunScenario( Seeds[0U] );
    const UInt64 Second = RunScenario( Seeds[1U] );
    const UInt64 Third  = RunScenario( Seeds[2U] );
    LUMEN_CHECK_NOT_EQ( First, Second );
    LUMEN_CHECK_NOT_EQ( First, Third );
}

} // namespace Lumen
