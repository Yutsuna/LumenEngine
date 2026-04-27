/**
 * @file Copyable.hpp
 * @brief Concepts for trivially copyable types.
 */

#pragma once

#include <type_traits>

namespace LumenEngine
{

namespace Concepts
{

    /**
     * @concept CTriviallyCopyable
     * @brief Checks if a type is trivially copyable, meaning it can be copied with a simple bitwise copy.
     */
    template <typename T>
    concept CTriviallyCopyable = std::is_trivially_copyable_v<T>;

} // namespace Concepts

} // namespace LumenEngine
