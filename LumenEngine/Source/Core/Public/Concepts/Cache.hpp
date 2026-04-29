/**
 * @file Cache.hpp
 * @brief Concepts for cache eviction policies in LumenEngine
 */

#pragma once

#include "Concepts/ConvertibleTo.hpp"
#include "Concepts/DefaultInitializable.hpp"
#include "Concepts/SameAs.hpp"

namespace LumenEngine
{

namespace Concepts
{

    /**
     * @concept CEvictionPolicy
     * @brief Constrains a stateful eviction policy
     *
     * @tparam PolicyType The eviction policy type
     * @tparam KeyType    The key type used by the cache
     * @tparam ValueType  The value type used by the cache
     */
    template <typename PolicyType, typename KeyType, typename ValueType>
    concept CEvictionPolicy = CDefaultInitializable<PolicyType> and requires( PolicyType &Policy, const KeyType &Key, const ValueType &Value ) {
        { Policy.OnInsert( Key, Value ) } -> CSameAs<void>;
        { Policy.OnAccess( Key ) } -> CSameAs<void>;
        { Policy.OnErase( Key ) } -> CSameAs<void>;
        { Policy.OnClear() } -> CSameAs<void>;
        { Policy.Victim() } -> CConvertibleTo<const KeyType &>;
    };

} // namespace Concepts

} // namespace LumenEngine
