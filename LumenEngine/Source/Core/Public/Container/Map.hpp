/**
 * @file Map.hpp
 * @brief Declaration of the FMap class for key-value pair storage and retrieval.
 */

#pragma once

#include <unordered_map>

namespace LumenEngine
{

// TODO: Implement custom map class
template <typename KeyType, typename ValueType> using TMap = std::unordered_map<KeyType, ValueType>;

} // namespace LumenEngine
