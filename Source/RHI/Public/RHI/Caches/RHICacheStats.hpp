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

namespace RHI
{

    /**
     * @struct FRHICacheStats
     * @brief Value snapshot of cache lookups, storage, and eviction activity.
     * @details All counters are monotonically increasing within a cache's lifetime
     *          except EntryCount and Bytes, which reflect current occupancy. The
     *          snapshot owns its values and is safe to retain after the source
     *          cache changes or is destroyed.
     */
    struct RHI_EXPORT FRHICacheStats
    {
        /** @brief Number of successful lookups. */
        UInt64 Hits = 0ULL;
        /** @brief Number of lookups that found no entry. */
        UInt64 Misses = 0ULL;
        /** @brief Number of entries inserted. */
        UInt64 Insertions = 0ULL;
        /** @brief Number of entries removed to reclaim capacity. */
        UInt64 Evictions = 0ULL;
        /** @brief Number of entries currently held. */
        UInt64 EntryCount = 0ULL;
        /** @brief Bytes currently occupied by held entries. */
        UInt64 Bytes = 0ULL;

        /**
         * @brief Computes the hit ratio from Hits and Misses.
         * @return A value in [0, 1], or zero when no lookup occurred.
         */
        [[nodiscard]] Float64 GetHitRate () const noexcept;
    };

    /**
     * @class FRHICacheStatsTracker
     * @brief Synchronized counters shared by object and pipeline caches.
     * @details Every mutator and Snapshot is serialized by an internal recursive
     *          shared mutex, so a tracker may be called concurrently from any
     *          cache client thread without external locking. The tracker owns no
     *          cached objects; it only aggregates counts and byte totals.
     */
    class RHI_EXPORT FRHICacheStatsTracker
    {
    public:

        /**
         * @brief Records one lookup.
         * @param bHit True when the lookup found an entry, false otherwise.
         */
        void OnLookup ( Bool bHit ) noexcept;

        /**
         * @brief Records an insertion and adds its size to occupancy.
         * @param InBytes Byte size of the inserted entry.
         */
        void OnInsert ( UInt64 InBytes ) noexcept;

        /**
         * @brief Records an eviction and removes its size from occupancy.
         * @param InBytes Byte size of the evicted entry; occupancy is clamped at
         *        zero if the value exceeds the tracked total.
         */
        void OnEvict ( UInt64 InBytes ) noexcept;

        /**
         * @brief Records an explicit removal without counting an eviction.
         * @param InBytes Byte size of the removed entry; occupancy is clamped at
         *        zero if the value exceeds the tracked total.
         */
        void OnRemove ( UInt64 InBytes ) noexcept;

        /** @brief Resets every counter and occupancy value to zero. */
        void Reset () noexcept;

        /**
         * @brief Captures the current counters as an independent value.
         * @return A coherent snapshot taken while the internal mutex is held.
         */
        [[nodiscard]] FRHICacheStats Snapshot () const noexcept;

    private:

        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
        FRHICacheStats Stats;
    };

} // namespace RHI

} // namespace Lumen
