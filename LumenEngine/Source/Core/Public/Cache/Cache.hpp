/**
 * @file Cache.hpp
 * @brief Cache system for storing and retrieving data based on keys.
 */

#pragma once

#include "Concepts/Cache.hpp"
#include "Concepts/Hash.hpp"
#include "Concepts/MoveConstructible.hpp"

#include "Container/Optional.hpp"
#include "NonCopyable.hpp"
#include "NonMovable.hpp"

#include "Cache/CachePolicy.hpp"

#include <limits>

namespace LumenEngine
{

template <Concepts::CCacheKey KeyType,
          Concepts::CMoveConstructible ValueType,
          typename PolicyType = Cache::FNoEviction<KeyType, ValueType>,
          typename HashFunc   = Hash::TStdHashAdapter<KeyType>>
    requires Concepts::CEvictionPolicy<PolicyType, KeyType, ValueType>
class TCache final : public FNonCopyable, public FNonMovable
{
public:

    static constexpr USize Unbounded = std::numeric_limits<USize>::max();

public:

    TCache () noexcept = default;

    /**
     * @brief Construct a bounded cache with a maximum capacity.
     * @param InMaxSize  Maximum number of entries before eviction occurs.
     *                   Pass Unbounded (default) for no limit.
     */
    explicit TCache ( const USize InMaxSize ) noexcept;

public:

    /**
     * @brief Look up a key and return a pointer to the cached value, or nullptr.
     *
     * The returned pointer is valid only while the caller holds a shared lock
     * obtained externally
     *
     * Prefer @ref TryGetCopy for cross-thread safety.
     *
     * @param InKey  The key to look up.
     * @return Const pointer to the value, or nullptr if not found.
     *
     * @note This overload is lock-free from the caller's perspective: it acquires
     *       a shared (read) lock internally and releases it before returning the
     *       pointer.  The pointer is valid as long as no write occurs.  For safe
     *       cross-thread usage copy the value out with TryGetCopy().
     */
    [[nodiscard]] const ValueType *TryGet ( const KeyType &InKey ) noexcept;

    /**
     * @brief Look up a key and return an optional copy of the cached value.
     *
     * Thread-safe: the value is copied under the shared lock.
     *
     * @param InKey  The key to look up.
     * @return Optional containing a copy of the value, or nullopt if not found.
     */
    [[nodiscard]] TOptional<ValueType> TryGetCopy ( const KeyType &InKey ) noexcept;

    /** @return true if the cache contains the given key */
    [[nodiscard]] Bool Contains ( const KeyType &InKey ) const noexcept;

public:

    /**
     * @brief Insert or update a key-value pair.
     *
     * If the cache is at capacity the eviction policy's Victim() is called and
     * that entry is removed before insertion.
     *
     * @param InKey    The key.
     * @param InValue  The value (copied).
     */
    void Put ( const KeyType &InKey, const ValueType &InValue );

    /**
     * @brief Insert or update a key-value pair (move overload).
     *
     * @param InKey    The key.
     * @param InValue  The value (moved).
     */
    void Put ( const KeyType &InKey, ValueType &&InValue ) noexcept;

    /**
     * @brief Remove a single entry. No-op if the key is absent.
     * @param InKey  Key to remove.
     * @return True if an entry was removed.
     */
    Bool Erase ( const KeyType &InKey ) noexcept;

    /**
     * @brief Remove a single entry. No-op if the key is absent.
     * @param InKey  Key to remove.
     * @return True if an entry was removed.
     */
    Bool Erase ( const KeyType &InKey ) noexcept;

    /**
     * @brief Remove all entries.
     * @return Number of entries removed.
     */
    USize Clear () noexcept;

private:

    /** Maximum number of entries before eviction occurs; Unbounded (default) for no limit */
    USize MaxSize{ Unbounded };

    /** Underlying storage for key-value pairs */
    TMap<KeyType, ValueType, HashFunc> Storage;

    /** Readers/writer mutex ; many concurrent readers, one exclusive writer */
    mutable FSharedMutex Mutex;

    /** Pluggable eviction policy instance */
    PolicyType EvictionPolicy;

    /** Atomic hit / miss / eviction / insert counters */
    Cache::FCacheCounters Counters;
};

} // namespace LumenEngine
