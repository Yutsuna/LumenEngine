/**
 * @file DefaultInitializable.hpp
 * @brief Concept for DefaultInitializable types in LumenEngine
 */

#pragma once

#include <concepts>

namespace LumenEngine
{

namespace Concepts
{

    /**
     * @concept CDefaultInitializable
     * @brief Constrains types that can be default initialized
     * @tparam ObjectType The type to check for default initializability
     */
    template <typename ObjectType>
    concept CDefaultInitializable = std::default_initializable<ObjectType>;

} // namespace Concepts

} // namespace LumenEngine
