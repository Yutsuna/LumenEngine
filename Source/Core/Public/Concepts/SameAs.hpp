///
/// @file SameAs.hpp
/// @brief Concept to check if a type is the same as another type.
///

#pragma once

#include <type_traits>

namespace Lumen
{
namespace Concepts
{
    ///
    /// @concept CIsSame
    /// @brief Checks if one type is the same as another type.
    /// @tparam T1 The first type.
    /// @tparam T2 The second type.
    ///
    template <typename T1, typename T2>
    concept CIsSame = std::is_same_v<T1, T2>;

    ///
    /// @concept CSameAs
    /// @brief Checks if a type is exactly the same as a reference type.
    /// @tparam T1 The type to check.
    /// @tparam T2 The reference type to compare against.
    ///
    template <typename T1, typename T2>
    concept CSameAs = std::is_same_v<T1, T2>;

} // namespace Concepts
} // namespace Lumen