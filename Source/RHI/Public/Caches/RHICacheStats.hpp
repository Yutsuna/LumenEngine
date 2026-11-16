/**
 * @file RHICacheStats.hpp
 * @brief Defines coherent cache statistics and a thread-safe counter tracker.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <HAL/PlatformMutex.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

/**
 * @struct FRHICacheStats
 * @brief Value snapshot of cache lookups, storage, and eviction activity.
 */
struct RHI_EXPORT FRHICacheStats
{
    UInt64 Hits       = 0ULL;
    UInt64 Misses     = 0ULL;
    UInt64 Insertions = 0ULL;
    UInt64 Evictions  = 0ULL;
    UInt64 EntryCount = 0ULL;
    UInt64 Bytes      = 0ULL;

    /** @return Hit ratio in [0,1], or zero when no lookup occurred. */
    [[nodiscard]] Float64 GetHitRate () const noexcept;
};

/**
 * @class FRHICacheStatsTracker
 * @brief Synchronized counters shared by object and pipeline caches.
 */
class RHI_EXPORT FRHICacheStatsTracker
{
public:

    void OnLookup ( Bool bHit ) noexcept;
    void OnInsert ( UInt64 InBytes ) noexcept;
    void OnEvict ( UInt64 InBytes ) noexcept;
    void OnRemove ( UInt64 InBytes ) noexcept;
    void Reset () noexcept;
    [[nodiscard]] FRHICacheStats Snapshot () const noexcept;

private:

    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    FRHICacheStats Stats;
};

} // namespace Lumen
