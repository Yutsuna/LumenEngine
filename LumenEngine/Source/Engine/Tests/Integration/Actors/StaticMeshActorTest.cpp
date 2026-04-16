/**
 * @file StaticMeshActorTest.cpp
 * @brief Integration and scale tests for AStaticMeshActor and FWorld performance.
 */

#include "Actors/StaticMeshActor.hpp"
#include "World/SpatialRegistry.hpp"
#include "World/World.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <numeric>

/**
 * @brief Integration test for AStaticMeshActor scaling and performance.
 * Validates FWorld::Tick with 50,000 instances to measure the impact of Publish.
 */
TEST( EngineIntegration, StaticMeshActorScaleTest )
{
    using namespace LumenEngine;
    using namespace LumenEngine::Engine;

    FWorld World;
    constexpr USize NumActors      = 50000U;
    constexpr USize WarmupTicks    = 5U;
    constexpr USize IterationCount = 10U;

    TVector<TSharedRef<AStaticMeshActor>> SpannedActors;
    SpannedActors.reserve( NumActors );

    /** 1. Spawn 50,000 actors */
    for ( USize Index = 0U; Index < NumActors; ++Index )
    {
        SpannedActors.push_back( World.SpawnActor<AStaticMeshActor>() );
    }

    /** 2. Warmup ticks to stabilize allocator and caches */
    for ( USize Index = 0U; Index < WarmupTicks; ++Index )
    {
        World.Tick( 0.016F );
    }

    /** 3. Measure Publish performance with ZERO updates (Averaged, Nanoseconds) */
    {
        TVector<Int64> Timings;
        for ( USize Iter = 0U; Iter < IterationCount; ++Iter )
        {
            const auto Start = std::chrono::high_resolution_clock::now();
            FSpatialRegistry::Get().Publish();
            const auto End = std::chrono::high_resolution_clock::now();
            Timings.push_back( std::chrono::duration_cast<std::chrono::nanoseconds>( End - Start ).count() );
        }

        const auto Avg = std::accumulate( Timings.begin(), Timings.end(), 0L ) / static_cast<Int64>( IterationCount );
        std::cout << "[ PERFORMANCE ] Avg Publish (0/50,000 updates): " << Avg << " ns" << std::endl;
    }

    /** 4. Measure Publish performance with ONE update (Averaged, Nanoseconds) */
    {
        TVector<Int64> Timings;
        for ( USize Iter = 0U; Iter < IterationCount; ++Iter )
        {
            SpannedActors[0]->SetTransform( Maths::FMatrix4x4f::Translate( { static_cast<Float32>( Iter ), 0.0F, 0.0F } ) );

            const auto Start = std::chrono::high_resolution_clock::now();
            FSpatialRegistry::Get().Publish();
            const auto End = std::chrono::high_resolution_clock::now();
            Timings.push_back( std::chrono::duration_cast<std::chrono::nanoseconds>( End - Start ).count() );
        }

        const auto Avg = std::accumulate( Timings.begin(), Timings.end(), 0L ) / static_cast<Int64>( IterationCount );
        std::cout << "[ PERFORMANCE ] Avg Publish (1/50,000 updates): " << Avg << " ns" << std::endl;
    }

    /** 5. Measure Publish performance with 100% updates (Averaged, Nanoseconds) */
    {
        TVector<Int64> Timings;
        for ( USize Iter = 0U; Iter < IterationCount; ++Iter )
        {
            for ( USize Index = 0U; Index < NumActors; ++Index )
            {
                SpannedActors[Index]->SetTransform( Maths::FMatrix4x4f::Translate( { static_cast<Float32>( Index + Iter ), 0.0F, 0.0F } ) );
            }

            const auto Start = std::chrono::high_resolution_clock::now();
            FSpatialRegistry::Get().Publish();
            const auto End = std::chrono::high_resolution_clock::now();
            Timings.push_back( std::chrono::duration_cast<std::chrono::nanoseconds>( End - Start ).count() );
        }

        const auto Avg = std::accumulate( Timings.begin(), Timings.end(), 0L ) / static_cast<Int64>( IterationCount );
        std::cout << "[ PERFORMANCE ] Avg Publish (50,000/50,000 updates): " << Avg << " ns" << std::endl;
    }
}
