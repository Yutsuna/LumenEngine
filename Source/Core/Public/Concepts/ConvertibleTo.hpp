///
/// @file ConvertibleTo.hpp
/// @brief Concept to check if a type is convertible to another type.
///

#pragma once

#include <type_traits>

namespace Lumen
{

namespace Concepts
{
    ///
    /// @concept CConvertibleTo
    /// @brief Checks if one type is convertible to another type.
    /// @tparam From The Source type.
    /// @tparam To The Target type.
    ///
    template <typename From, typename To>
    concept CConvertibleTo = std::is_convertible_v<From, To>;
} // namespace Concepts

} // namespace Lumen