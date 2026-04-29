/**
 * @file MoveConstructible.hpp
 * @brief Concept for MoveConstructible types in LumenEngine
 */

#pragma once

#include <concepts>

namespace LumenEngine
{

namespace Concepts
{
    /**
     * @concept CMoveConstructible
     * @brief Constrains types that can be move constructed
     * @tparam ObjectType The type to check for move constructibility
     */
    template <typename ObjectType>
    concept CMoveConstructible = std::move_constructible<ObjectType>;

} // namespace Concepts

} // namespace LumenEngine
