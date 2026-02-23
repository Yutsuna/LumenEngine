/**
 * @file Queue.hpp
 * @brief Definition of the FQueue class, which represents a thread-safe queue.
 */

#pragma once

#include <queue>

template <typename ObjectType> using FQueue = std::queue<ObjectType>;
