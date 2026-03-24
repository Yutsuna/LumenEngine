/**
 * @file ConvertibleTo.hpp
 * @brief Concepts for type conversion checks.
 */

#pragma once

#include <type_traits>

namespace LumenEngine
{

namespace Concepts
{

    /**
     * @concept CConvertibleTo
     * @brief Checks if one type is convertible to another.
     * @param From The source type.
     * @param To The target type.
     */
    template <typename From, typename To>
    concept CConvertibleTo = std::is_convertible_v<From, To>;

} // namespace Concepts

} // namespace LumenEngine
