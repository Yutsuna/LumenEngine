/**
 * @file RHICacheStats.cpp
 * @brief Implements coherent cache statistics snapshots.
 */

#include <HAL/LockGuard.hpp>
#include <RHI/Caches/RHICacheStats.hpp>

Lumen::Float64 Lumen::RHI::FRHICacheStats::GetHitRate () const noexcept
{
    const UInt64 Lookups = Hits + Misses;
    return Lookups == 0ULL ? 0.0 : static_cast<Float64>( Hits ) / static_cast<Float64>( Lookups );
}

void Lumen::RHI::FRHICacheStatsTracker::OnLookup ( const Bool bHit ) noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( bHit )
    {
        ++Stats.Hits;
    }
    else
    {
        ++Stats.Misses;
    }
}

void Lumen::RHI::FRHICacheStatsTracker::OnInsert ( const UInt64 InBytes ) noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    ++Stats.Insertions;
    ++Stats.EntryCount;
    Stats.Bytes += InBytes;
}

void Lumen::RHI::FRHICacheStatsTracker::OnEvict ( const UInt64 InBytes ) noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    ++Stats.Evictions;
    Stats.EntryCount = Stats.EntryCount == 0ULL ? 0ULL : Stats.EntryCount - 1ULL;
    Stats.Bytes      = InBytes <= Stats.Bytes ? Stats.Bytes - InBytes : 0ULL;
}

void Lumen::RHI::FRHICacheStatsTracker::OnRemove ( const UInt64 InBytes ) noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Stats.EntryCount = Stats.EntryCount == 0ULL ? 0ULL : Stats.EntryCount - 1ULL;
    Stats.Bytes      = InBytes <= Stats.Bytes ? Stats.Bytes - InBytes : 0ULL;
}

void Lumen::RHI::FRHICacheStatsTracker::Reset () noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Stats = {};
}

Lumen::RHI::FRHICacheStats Lumen::RHI::FRHICacheStatsTracker::Snapshot () const noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Stats;
}
