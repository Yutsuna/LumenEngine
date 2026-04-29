/**
 * @file CacheStats.cpp
 * @brief Cache Statistics Tracking in LumenEngine
 */

#include "Cache/CacheStats.hpp"

void LumenEngine::Cache::FCacheCounters::RecordHit () noexcept
{
    HitCount.fetch_add( 1ULL, std::memory_order_relaxed );
}

void LumenEngine::Cache::FCacheCounters::RecordMiss () noexcept
{
    MissCount.fetch_add( 1ULL, std::memory_order_relaxed );
}

void LumenEngine::Cache::FCacheCounters::RecordEviction () noexcept
{
    EvictionCount.fetch_add( 1ULL, std::memory_order_relaxed );
}

void LumenEngine::Cache::FCacheCounters::RecordInsert () noexcept
{
    InsertCount.fetch_add( 1ULL, std::memory_order_relaxed );
}

void LumenEngine::Cache::FCacheCounters::Reset () noexcept
{
    HitCount.store( 0ULL, std::memory_order_relaxed );
    MissCount.store( 0ULL, std::memory_order_relaxed );
    EvictionCount.store( 0ULL, std::memory_order_relaxed );
    InsertCount.store( 0ULL, std::memory_order_relaxed );
}

[[nodiscard]] LumenEngine::Cache::FCacheStats LumenEngine::Cache::FCacheCounters::Snapshot () const noexcept
{
    return FCacheStats{
        .Hits      = HitCount.load( std::memory_order_relaxed ),
        .Misses    = MissCount.load( std::memory_order_relaxed ),
        .Evictions = EvictionCount.load( std::memory_order_relaxed ),
        .Inserts   = InsertCount.load( std::memory_order_relaxed ),
    };
}
