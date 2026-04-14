/**
 * @file Queue.hpp
 * @brief Definition of the TQueue class, which represents a thread-safe queue.
 */

#pragma once

#include <queue>

template <typename ObjectType> using TQueue = std::queue<ObjectType>;
