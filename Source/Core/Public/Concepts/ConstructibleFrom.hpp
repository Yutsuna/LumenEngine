///
/// @file ConstructibleFrom.hpp
/// @brief Checks if one type is constructible to another type.
///

#pragma once

#include <type_traits>

namespace Lumen
{
namespace Concepts
{
    ///
    /// @concept CConstructibleFrom
    /// @brief Checks if one type is constructible to another type.
    /// @tparam From The Source type.
    /// @tparam To The Target type.
    ///
    template <typename From, typename... To>
    concept CConstructibleFrom = std::is_constructible_v<From, To...>;

} // namespace Concepts
} // namespace Lumen