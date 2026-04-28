/**
 * @file CachePolicy.inl
 * @brief Inline implementations for cache eviction policies in LumenEngine
 */

#pragma once

#include "Cache/CachePolicy.hpp"

#include <cassert>

/**
 * FNoEviction
 */

template <typename KeyType, typename ValueType>
void LumenEngine::Cache::FNoEviction<KeyType, ValueType>::OnInsert ( const KeyType &LUMEN_UNUSED InKey, const ValueType &LUMEN_UNUSED InValue ) noexcept
{
    /* No state to update */
}

template <typename KeyType, typename ValueType> void LumenEngine::Cache::FNoEviction<KeyType, ValueType>::OnAccess ( const KeyType &LUMEN_UNUSED InKey ) noexcept
{
    /* No state to update */
}

template <typename KeyType, typename ValueType> void LumenEngine::Cache::FNoEviction<KeyType, ValueType>::OnErase ( const KeyType &LUMEN_UNUSED InKey ) noexcept
{
    /* No state to update */
}

template <typename KeyType, typename ValueType> void LumenEngine::Cache::FNoEviction<KeyType, ValueType>::OnClear () noexcept
{
    /* No state to update */
}

template <typename KeyType, typename ValueType> const KeyType &LumenEngine::Cache::FNoEviction<KeyType, ValueType>::Victim () const noexcept
{
    assert( false and "FNoEviction::Victim() called on empty or unbounded cache" );
    static const KeyType DummyKey{};

    return DummyKey;
}

/**
 * FLruPolicy
 */

template <typename TKey, typename TValue> void LumenEngine::Cache::FLruPolicy<TKey, TValue>::OnInsert ( const TKey &InKey, const TValue &LUMEN_UNUSED InValue )
{
    Order.push_front( InKey );
    IteratorMap.emplace( InKey, Order.begin() );
}

template <typename TKey, typename TValue> void LumenEngine::Cache::FLruPolicy<TKey, TValue>::OnAccess ( const TKey &InKey )
{
    if ( auto It = IteratorMap.find( InKey ); It != IteratorMap.end() )
    {
        Order.splice( Order.begin(), Order, It->second );
    }
}

template <typename TKey, typename TValue> void LumenEngine::Cache::FLruPolicy<TKey, TValue>::OnErase ( const TKey &InKey )
{
    if ( auto It = IteratorMap.find( InKey ); It != IteratorMap.end() )
    {
        Order.erase( It->second );
        IteratorMap.erase( It );
    }
}

template <typename TKey, typename TValue> void LumenEngine::Cache::FLruPolicy<TKey, TValue>::OnClear ()
{
    Order.clear();
    IteratorMap.clear();
}

template <typename TKey, typename TValue> const TKey &LumenEngine::Cache::FLruPolicy<TKey, TValue>::Victim () const
{
    assert( not Order.empty() and "FLruPolicy::Victim() called on empty LRU list" );
    return Order.back();
}
