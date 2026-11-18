/**
 * @file RHIObjectCache.hpp
 * @brief Declares a synchronized bounded object cache with explicit eviction.
 */
#pragma once

#include <Caches/RHICacheStats.hpp>
#include <Memory/SharedPtr.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Function.hpp>
#include <Templates/Map.hpp>
#include <Templates/Optional.hpp>

#include <Types/RHIError.hpp>
#include <Types/RHIHash.hpp>

namespace Lumen
{

/**
 * @enum ERHICacheEvictionPolicy
 * @brief Behavior when a bounded cache is full.
 */
enum class ERHICacheEvictionPolicy : UInt8
{
    None,
    LeastRecentlyUsed
};

/**
 * @class TRHIObjectCache
 * @brief Thread-safe shared-object cache; factories run outside the internal lock.
 * @details A concurrent duplicate factory result is discarded in favor of the first stored value.
 */
template <typename KeyType, typename ValueType, typename HashType = TRHIValueHasher<KeyType>> class TRHIObjectCache
{
public:

    explicit TRHIObjectCache ( USize InMaxEntries,
                               ERHICacheEvictionPolicy InPolicy = ERHICacheEvictionPolicy::LeastRecentlyUsed ) noexcept;
    /**
     * @brief Looks up a cached object without creating it.
     * @param InKey Key to look up.
     * @return A shared pointer to the cached value on a hit, or empty on a miss.
     */
    [[nodiscard]] TOptional<TSharedPtr<ValueType>> Find ( const KeyType &InKey ) noexcept;

    /**
     * @brief Returns the cached value, invoking InFactory on a miss.
     * @param InKey Key to look up.
     * @param InFactory Factory run without the internal lock when the key is absent.
     * @param InBytes Byte size charged to cache occupancy.
     * @return The cached or newly created value, or the factory error; concurrent duplicate results are discarded.
     */
    [[nodiscard]] TExpected<TSharedPtr<ValueType>, FRHIError>
    GetOrCreate ( const KeyType &InKey,
                  TFunction<TExpected<TSharedRef<ValueType>, FRHIError>()> InFactory,
                  UInt64 InBytes = sizeof( ValueType ) ) noexcept;

    /**
     * @brief Inserts or replaces a value under a key.
     * @param InKey Key to store.
     * @param InValue Complete value to retain.
     * @param InBytes Byte size charged to occupancy.
     * @return Success, or OutOfMemory if capacity cannot be made available.
     */
    TExpected<void, FRHIError> Store ( const KeyType &InKey, TSharedRef<ValueType> InValue, UInt64 InBytes ) noexcept;

    /**
     * @brief Removes the entry for a key.
     * @param InKey Key to remove.
     */
    void Remove ( const KeyType &InKey ) noexcept;

    /**
     * @brief Removes all entries and resets statistics.
     */
    void Clear () noexcept;

    /**
     * @brief Returns a coherent snapshot of cache activity.
     * @return Current counters and occupancy.
     */
    [[nodiscard]] FRHICacheStats GetStats () const noexcept;

    /**
     * @brief Returns the number of entries currently held.
     * @return Current occupancy.
     */
    [[nodiscard]] USize GetEntryCount () const noexcept;

private:

    struct FEntry
    {
        TSharedPtr<ValueType> Value;
        UInt64 Bytes    = 0ULL;
        UInt64 LastUsed = 0ULL;
    };

    /**
     * @brief Evicts an entry when the cache is full.
     * @return True when capacity is available after the call.
     * @note Private helper; called only with the internal lock held.
     */
    [[nodiscard]] Bool MakeRoom () noexcept;
    /**
     * @brief Evicts one entry according to the configured policy.
     * @note Private helper; called only with the internal lock held.
     */
    void EvictOne () noexcept;

private:

    USize MaxEntries;
    ERHICacheEvictionPolicy Policy;
    TMap<KeyType, FEntry, HashType> Entries;
    UInt64 Sequence = 0ULL;
    FRHICacheStatsTracker Stats;
    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};

} // namespace Lumen

#include "Inline/RHIObjectCache.inl"
