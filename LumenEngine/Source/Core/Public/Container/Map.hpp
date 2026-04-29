/**
 * @file Map.hpp
 * @brief Declaration of the FMap class for key-value pair storage and retrieval.
 */

#pragma once

#include <unordered_map>

namespace LumenEngine
{

template <typename KeyType,
          typename ValueType,
          typename HashFunc  = std::hash<KeyType>,
          typename EqualFunc = std::equal_to<KeyType>,
          typename Allocator = std::allocator<std::pair<const KeyType, ValueType>>>
using TMap = std::unordered_map<KeyType, ValueType, HashFunc, EqualFunc, Allocator>;

} // namespace LumenEngine
