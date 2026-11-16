/**
 * @file RHIObjectCache.hpp
 * @brief Declares a synchronized bounded object cache with explicit eviction.
 */
#pragma once

#include "Memory/SharedPtr.hpp"
#include "Types/RHIError.hpp"
#include <Caches/RHICacheStats.hpp>
#include <Memory/SharedPtr.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Function.hpp>
#include <Templates/Map.hpp>
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
    [[nodiscard]] TSharedPtr<ValueType> Find ( const KeyType &InKey ) noexcept;
    [[nodiscard]] TExpected<TSharedPtr<ValueType>, FRHIError>
    GetOrCreate ( const KeyType &InKey,
                  TFunction<TExpected<TSharedRef<ValueType>, FRHIError>()> InFactory,
                  UInt64 InBytes = sizeof( ValueType ) ) noexcept;
    TExpected<void, FRHIError> Store ( const KeyType &InKey, TSharedRef<ValueType> InValue, UInt64 InBytes ) noexcept;
    void Remove ( const KeyType &InKey ) noexcept;
    void Clear () noexcept;
    [[nodiscard]] FRHICacheStats GetStats () const noexcept;
    [[nodiscard]] USize GetEntryCount () const noexcept;

private:

    struct FEntry
    {
        TSharedPtr<ValueType> Value;
        UInt64 Bytes    = 0ULL;
        UInt64 LastUsed = 0ULL;
    };

    [[nodiscard]] Bool MakeRoom () noexcept;
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
