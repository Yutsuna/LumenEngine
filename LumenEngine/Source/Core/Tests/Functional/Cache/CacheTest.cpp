/**
 * @file CacheTest.cpp
 * @brief Unit tests for TCache<> with FNoEviction and FLruPolicy.
 */

#include "Cache/Cache.hpp"
#include "Container/Vector.hpp"
#include "Hash/Hash.hpp"

#include "Container/String.hpp"
#include "CoreTypes.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <barrier>

namespace LumenEngine
{

namespace Cache
{

    using FIntCache   = TCache<Int32, FString>;
    using FStrCache   = TCache<FString, Int32>;
    using FLruCache64 = TCache<Int32, FString, FLruPolicy<Int32, FString>>;

    TEST( TCache, DefaultConstructedIsEmpty )
    {
        FIntCache Cache;
        EXPECT_TRUE( Cache.IsEmpty() );
        EXPECT_EQ( Cache.Size(), 0U );
    }

    TEST( TCache, PutAndTryGet )
    {
        FIntCache Cache;
        Cache.Put( 1, "one" );

        const FString *Ptr = Cache.TryGet( 1 );
        ASSERT_NE( Ptr, nullptr );
        EXPECT_EQ( *Ptr, "one" );
    }

    TEST( TCache, TryGetMissingKeyReturnsNull )
    {
        FIntCache Cache;
        EXPECT_EQ( Cache.TryGet( 42 ), nullptr );
    }

    TEST( TCache, TryGetCopy )
    {
        FIntCache Cache;
        Cache.Put( 7, "seven" );

        TOptional<FString> Copy = Cache.TryGetCopy( 7 );
        ASSERT_TRUE( Copy.has_value() );
        EXPECT_EQ( *Copy, "seven" );
    }

    TEST( TCache, TryGetCopyMissingKey )
    {
        FIntCache Cache;
        EXPECT_FALSE( Cache.TryGetCopy( 99 ).has_value() );
    }

    TEST( TCache, Contains )
    {
        FIntCache Cache;
        EXPECT_FALSE( Cache.Contains( 5 ) );
        Cache.Put( 5, "five" );
        EXPECT_TRUE( Cache.Contains( 5 ) );
    }

    TEST( TCache, SizeTracksEntries )
    {
        FIntCache Cache;
        Cache.Put( 1, "a" );
        Cache.Put( 2, "b" );
        Cache.Put( 3, "c" );
        EXPECT_EQ( Cache.Size(), 3U );
    }

    TEST( TCache, PutOverwritesExistingValue )
    {
        FIntCache Cache;
        Cache.Put( 1, "first" );
        Cache.Put( 1, "second" );

        EXPECT_EQ( Cache.Size(), 1U );
        const FString *Ptr = Cache.TryGet( 1 );
        ASSERT_NE( Ptr, nullptr );
        EXPECT_EQ( *Ptr, "second" );
    }

    TEST( TCache, PutMoveOverload )
    {
        FIntCache Cache;
        FString Val = "move_me";
        Cache.Put( 10, std::move( Val ) );

        EXPECT_TRUE( Cache.Contains( 10 ) );
    }

    TEST( TCache, EraseExistingKey )
    {
        FIntCache Cache;
        Cache.Put( 1, "one" );
        EXPECT_TRUE( Cache.Erase( 1 ) );
        EXPECT_FALSE( Cache.Contains( 1 ) );
        EXPECT_EQ( Cache.Size(), 0U );
    }

    TEST( TCache, EraseMissingKeyReturnsFalse )
    {
        FIntCache Cache;
        EXPECT_FALSE( Cache.Erase( 999 ) );
    }

    TEST( TCache, ClearRemovesAllEntries )
    {
        FIntCache Cache;
        Cache.Put( 1, "a" );
        Cache.Put( 2, "b" );

        const USize Removed = Cache.Clear();
        EXPECT_EQ( Removed, 2U );
        EXPECT_TRUE( Cache.IsEmpty() );
    }

    TEST( TCache, HitMissCounters )
    {
        FIntCache Cache;
        Cache.Put( 1, "x" );

        Cache.TryGet( 1 );
        Cache.TryGet( 2 );
        Cache.TryGet( 1 );

        const FCacheStats Stats = Cache.GetStats();
        EXPECT_EQ( Stats.Hits, 2U );
        EXPECT_EQ( Stats.Misses, 1U );
    }

    TEST( TCache, InsertCounter )
    {
        FIntCache Cache;
        Cache.Put( 1, "a" );
        Cache.Put( 2, "b" );
        Cache.Put( 1, "a2" );

        EXPECT_EQ( Cache.GetStats().Inserts, 2U );
    }

    TEST( TCache, HitRatioCalculation )
    {
        FIntCache Cache;
        Cache.Put( 1, "v" );
        Cache.TryGet( 1 );
        Cache.TryGet( 2 );

        const FCacheStats Stats = Cache.GetStats();
        EXPECT_DOUBLE_EQ( Stats.HitRatio(), 0.5 );
    }

    TEST( TCache, HitRatioIsZeroWithNoLookups )
    {
        FIntCache Cache;
        EXPECT_DOUBLE_EQ( Cache.GetStats().HitRatio(), 0.0 );
    }

    TEST( TCache, ResetStats )
    {
        FIntCache Cache;
        Cache.Put( 1, "v" );
        Cache.TryGet( 1 );
        Cache.TryGet( 2 );

        Cache.ResetStats();
        EXPECT_EQ( Cache.GetStats().Hits, 0U );
        EXPECT_EQ( Cache.GetStats().Misses, 0U );
    }

    TEST( TCache, UnboundedCapacityStoresMany )
    {
        FIntCache Cache;
        for ( Int32 I = 0; I < 10000; ++I )
        {
            Cache.Put( I, FString( std::to_string( I ) ) );
        }
        EXPECT_EQ( Cache.Size(), 10000U );
    }

    TEST( FLruCache, EvictsLeastRecentlyUsed )
    {
        FLruCache64 Cache( 3 );

        Cache.Put( 1, "one" );
        Cache.Put( 2, "two" );
        Cache.Put( 3, "three" );

        Cache.TryGet( 1 );
        Cache.TryGet( 3 );

        Cache.Put( 4, "four" );

        EXPECT_EQ( Cache.Size(), 3U );
        EXPECT_FALSE( Cache.Contains( 2 ) );
        EXPECT_TRUE( Cache.Contains( 1 ) );
        EXPECT_TRUE( Cache.Contains( 3 ) );
        EXPECT_TRUE( Cache.Contains( 4 ) );
    }

    TEST( FLruCache, EvictionCounterIncremented )
    {
        FLruCache64 Cache( 2 );
        Cache.Put( 1, "a" );
        Cache.Put( 2, "b" );
        Cache.Put( 3, "c" );

        EXPECT_EQ( Cache.GetStats().Evictions, 1U );
    }

    TEST( FLruCache, UpdateDoesNotEvict )
    {
        FLruCache64 Cache( 2 );
        Cache.Put( 1, "a" );
        Cache.Put( 2, "b" );
        Cache.Put( 1, "a_updated" );

        EXPECT_EQ( Cache.Size(), 2U );
        EXPECT_EQ( Cache.GetStats().Evictions, 0U );

        const FString *Ptr = Cache.TryGet( 1 );
        ASSERT_NE( Ptr, nullptr );
        EXPECT_EQ( *Ptr, "a_updated" );
    }

    TEST( FLruCache, ClearResetsPolicy )
    {
        FLruCache64 Cache( 2 );
        Cache.Put( 1, "a" );
        Cache.Put( 2, "b" );
        Cache.Clear();

        Cache.Put( 3, "c" );
        Cache.Put( 4, "d" );
        EXPECT_EQ( Cache.Size(), 2U );
        EXPECT_EQ( Cache.GetStats().Evictions, 0U );
    }

    TEST( FLruCache, SetMaxSizeShrinks )
    {
        FLruCache64 Cache( 10 );
        for ( Int32 I = 0; I < 8; ++I )
        {
            Cache.Put( I, FString( std::to_string( I ) ) );
        }
        EXPECT_EQ( Cache.Size(), 8U );

        Cache.SetMaxSize( 4 );
        EXPECT_LE( Cache.Size(), 4U );
    }

    TEST( TCache, ConcurrentReadersAreThreadSafe )
    {
        FIntCache Cache;
        for ( Int32 I = 0; I < 100; ++I )
        {
            Cache.Put( I, FString( std::to_string( I ) ) );
        }

        constexpr Int32 NumThreads = 8;
        constexpr Int32 NumIter    = 5000;
        TAtomic<Bool> bAllCorrect{ true };
        std::barrier Start( NumThreads );
        TVector<std::thread> Threads;
        Threads.reserve( NumThreads );

        for ( Int32 T = 0; T < NumThreads; ++T )
        {
            Threads.emplace_back(
                [&] ()
                {
                    Start.arrive_and_wait();
                    for ( Int32 I = 0; I < NumIter; ++I )
                    {
                        const Int32 Key        = I % 100;
                        TOptional<FString> Val = Cache.TryGetCopy( Key );
                        if ( not Val.has_value() or *Val != std::to_string( Key ) )
                        {
                            bAllCorrect.store( false, std::memory_order_relaxed );
                        }
                    }
                } );
        }

        for ( auto &Thread : Threads )
        {
            Thread.join();
        }

        EXPECT_TRUE( bAllCorrect.load() );
    }

    TEST( TCache, ConcurrentWritersAreThreadSafe )
    {
        FIntCache Cache;
        constexpr Int32 NumThreads = 4;
        constexpr Int32 NumKeys    = 100;
        std::barrier Start( NumThreads );
        TVector<std::thread> Threads;
        Threads.reserve( NumThreads );

        for ( Int32 T = 0; T < NumThreads; ++T )
        {
            Threads.emplace_back(
                [&, T] ()
                {
                    Start.arrive_and_wait();
                    for ( Int32 I = 0; I < NumKeys; ++I )
                    {
                        Cache.Put( T * NumKeys + I, FString( std::to_string( I ) ) );
                    }
                } );
        }

        for ( auto &Thread : Threads )
        {
            Thread.join();
        }

        EXPECT_EQ( Cache.Size(), static_cast<USize>( NumThreads * NumKeys ) );
    }

    TEST( TCache, MixedReadersWritersStress )
    {
        FIntCache Cache;
        for ( Int32 I = 0; I < 50; ++I )
        {
            Cache.Put( I, FString( std::to_string( I ) ) );
        }

        constexpr Int32 NumWriters = 2;
        constexpr Int32 NumReaders = 6;
        constexpr Int32 NumIter    = 2000;
        std::barrier Start( NumWriters + NumReaders );
        TAtomic<Bool> bNoDataRace{ true };
        TVector<std::thread> Threads;

        for ( Int32 W = 0; W < NumWriters; ++W )
        {
            Threads.emplace_back(
                [&, W] ()
                {
                    Start.arrive_and_wait();
                    for ( Int32 I = 0; I < NumIter; ++I )
                    {
                        Cache.Put( W * 1000 + ( I % 50 ), FString( "writer" ) );
                    }
                } );
        }

        for ( Int32 R = 0; R < NumReaders; ++R )
        {
            Threads.emplace_back(
                [&] ()
                {
                    Start.arrive_and_wait();
                    for ( Int32 I = 0; I < NumIter; ++I )
                    {

                        ( void )Cache.TryGetCopy( I % 50 );
                    }
                } );
        }

        for ( auto &Thread : Threads )
        {
            Thread.join();
        }

        EXPECT_TRUE( bNoDataRace.load() );
    }

    TEST( FCacheStats, HitRatioEdgeCases )
    {
        FCacheStats Stats;
        EXPECT_DOUBLE_EQ( Stats.HitRatio(), 0.0 );

        Stats.Hits   = 3;
        Stats.Misses = 1;
        EXPECT_DOUBLE_EQ( Stats.HitRatio(), 0.75 );
    }

} // namespace Cache

} // namespace LumenEngine