/**
 * @file RHICacheStatsTests.cpp
 * @brief Tests cache counter transitions and coherent concurrent snapshots.
 */

#include <RHI/Caches/RHICacheStats.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHICacheStats, TracksLookupsStorageAndEviction )
        {
            FRHICacheStatsTracker Tracker;
            Tracker.OnLookup( true );
            Tracker.OnLookup( false );
            Tracker.OnInsert( 64ULL );
            Tracker.OnInsert( 32ULL );
            Tracker.OnEvict( 32ULL );
            Tracker.OnRemove( 64ULL );
            const FRHICacheStats Stats = Tracker.Snapshot();
            LUMEN_CHECK_EQ( Stats.Hits, 1ULL );
            LUMEN_CHECK_EQ( Stats.Misses, 1ULL );
            LUMEN_CHECK_EQ( Stats.Insertions, 2ULL );
            LUMEN_CHECK_EQ( Stats.Evictions, 1ULL );
            LUMEN_CHECK_EQ( Stats.EntryCount, 0ULL );
            LUMEN_CHECK_EQ( Stats.Bytes, 0ULL );
            LUMEN_CHECK_EQ( Stats.GetHitRate(), 0.5 );
            Tracker.Reset();
            LUMEN_CHECK_EQ( Tracker.Snapshot().Hits, 0ULL );
        }

        LUMEN_TEST( RHICacheStats, ConcurrentUpdatesRemainCoherent )
        {
            FRHICacheStatsTracker Tracker;
            TVector<FThread> Workers;
            for ( UInt32 Worker = 0U; Worker < 4U; ++Worker )
            {
                Workers.emplace_back( [&Tracker]
                {
                    Tracker.OnInsert( 16ULL );
                    for ( UInt32 Index = 0U; Index < 64U; ++Index )
                    {
                        Tracker.OnLookup( ( Index & 1U ) == 0U );
                    }
                } );
            }
            for ( auto &Worker : Workers )
            {
                Worker.Join();
            }
            const FRHICacheStats Stats = Tracker.Snapshot();
            LUMEN_CHECK_EQ( Stats.EntryCount, 4ULL );
            LUMEN_CHECK_EQ( Stats.Bytes, 64ULL );
            LUMEN_CHECK_EQ( Stats.Hits + Stats.Misses, 256ULL );
            LUMEN_CHECK_EQ( Stats.Hits, 128ULL );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
