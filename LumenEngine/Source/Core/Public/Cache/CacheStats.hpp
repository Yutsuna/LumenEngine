/**
 * @file CacheStats.hpp
 * @brief Cache Statistics Tracking in LumenEngine
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

namespace LumenEngine
{

namespace Cache
{

    /**
     * @struct FCacheStats
     * @brief Tracks cache performance metrics
     */
    struct LUMEN_ENGINE_API FCacheStats
    {
        UInt64 Hits      = 0ULL;
        UInt64 Misses    = 0ULL;
        UInt64 Evictions = 0ULL;
        UInt64 Inserts   = 0ULL;

        /** @return Hit ratio [O, 1]. Returns 0 if no accesses have been made. */
        [[nodiscard]] constexpr Float64 HitRatio () const noexcept;
    };

    /**
     * @class FCacheCounters
     * @brief Thread-safe atomic counters owned by TCache.
     */
    class FCacheCounters final
    {
    public:

        void RecordHit () noexcept;

        void RecordMiss () noexcept;

        void RecordEviction () noexcept;

        void RecordInsert () noexcept;

        void Reset () noexcept;

        [[nodiscard]] FCacheStats Snapshot () const noexcept;

    private:

        TAtomic<UInt64> HitCount{ 0ULL };
        TAtomic<UInt64> MissCount{ 0ULL };
        TAtomic<UInt64> EvictionCount{ 0ULL };
        TAtomic<UInt64> InsertCount{ 0ULL };
    };

} // namespace Cache

} // namespace LumenEngine

#include "Inline/CacheStats.inl"