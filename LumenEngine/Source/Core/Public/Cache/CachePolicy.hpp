/**
 * @file CachePolicy.hpp
 * @brief Cache Eviction Policies in LumenEngine
 */

#pragma once

#include "Container/List.hpp"
#include "Container/Map.hpp"
#include "Definitions.hpp"

namespace LumenEngine
{

namespace Cache
{

    template <typename KeyType, typename ValueType> class IEvictionPolicy
    {
    public:

        virtual ~IEvictionPolicy () = default;

    public:

        virtual void OnInsert ( const KeyType &InKey, const ValueType &InValue ) = 0;

        virtual void OnAccess ( const KeyType &InKey ) = 0;

        virtual void OnErase ( const KeyType &InKey ) = 0;

        virtual void OnClear () = 0;

        [[nodiscard]] virtual const KeyType &Victim () const = 0;
    };

    /**
     * @class FNoEviction
     * @brief Eviction policy stub for unbounded caches.
     *
     * Calling Victim() on an empty or unbounded cache is a programming error
     * and will assert in debug builds.
     *
     * @tparam KeyType    The key type used by the cache
     * @tparam ValueType  The value type used by the cache
     */
    template <typename KeyType, typename ValueType> class LUMEN_ENGINE_API FNoEviction final : public IEvictionPolicy<KeyType, ValueType>
    {
    public:

        ~FNoEviction () override = default;

    public:

        void OnInsert ( const KeyType &InKey, const ValueType &InValue ) noexcept override;

        void OnAccess ( const KeyType &InKey ) noexcept override;

        void OnErase ( const KeyType &InKey ) noexcept override;

        void OnClear () noexcept override;

        [[nodiscard]] const KeyType &Victim () const noexcept override;
    };

    /**
     * @class FLruPolicy
     * @brief Classic O(1) LRU eviction via a doubly-linked list + iterator map.
     *
     * - OnInsert / OnAccess move the key to the front of the MRU list.
     * - Victim()            returns the key at the back (least recently used).
     * - OnErase / OnClear   maintain list consistency.
     *
     * @tparam TKey    Key type (must be copyable).
     * @tparam TValue  Value type (unused by the policy itself).
     */
    template <typename TKey, typename TValue> class LUMEN_ENGINE_API FLruPolicy final : public IEvictionPolicy<TKey, TValue>
    {
    public:

        void OnInsert ( const TKey &InKey, const TValue &InValue ) override;

        void OnAccess ( const TKey &InKey ) override;

        void OnErase ( const TKey &InKey ) override;

        void OnClear () override;

        [[nodiscard]] const TKey &Victim () const override;

    private:

        /** MRU -> LRU order: front = most recently used, back = victim. */
        TList<TKey> Order;

        /** Maps each key to its iterator in Order for O(1) splice. */
        TMap<TKey, typename TList<TKey>::iterator> IteratorMap;
    };

} // namespace Cache

} // namespace LumenEngine

#include "Inline/CachePolicy.inl"
