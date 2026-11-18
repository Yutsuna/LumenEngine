/**
 * @file RHICacheStats.cpp
 * @brief Implements coherent cache statistics snapshots.
 */

#include <Caches/RHICacheStats.hpp>
#include <HAL/LockGuard.hpp>

Lumen::Float64 Lumen::FRHICacheStats::GetHitRate () const noexcept
{
    const UInt64 Lookups = Hits + Misses;
    return Lookups == 0ULL ? 0.0 : static_cast<Float64>( Hits ) / static_cast<Float64>( Lookups );
}

void Lumen::FRHICacheStatsTracker::OnLookup ( const Bool bHit ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( bHit )
    {
        ++Stats.Hits;
    }
    else
    {
        ++Stats.Misses;
    }
}

void Lumen::FRHICacheStatsTracker::OnInsert ( const UInt64 InBytes ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    ++Stats.Insertions;
    ++Stats.EntryCount;
    Stats.Bytes += InBytes;
}

void Lumen::FRHICacheStatsTracker::OnEvict ( const UInt64 InBytes ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    ++Stats.Evictions;
    Stats.EntryCount = Stats.EntryCount == 0ULL ? 0ULL : Stats.EntryCount - 1ULL;
    Stats.Bytes      = InBytes <= Stats.Bytes ? Stats.Bytes - InBytes : 0ULL;
}

void Lumen::FRHICacheStatsTracker::OnRemove ( const UInt64 InBytes ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Stats.EntryCount = Stats.EntryCount == 0ULL ? 0ULL : Stats.EntryCount - 1ULL;
    Stats.Bytes      = InBytes <= Stats.Bytes ? Stats.Bytes - InBytes : 0ULL;
}

void Lumen::FRHICacheStatsTracker::Reset () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Stats = {};
}

Lumen::FRHICacheStats Lumen::FRHICacheStatsTracker::Snapshot () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Stats;
}
