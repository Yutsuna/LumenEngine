///
/// @file TriviallyCopyable.hpp
/// @brief Concepts for trivially copyable types.
///

#pragma once

#include <type_traits>

namespace Lumen
{

namespace Concepts
{

    ///
    /// @concept CTriviallyCopyable
    /// @brief Checks if a type is trivially copyable, meaning it can be copied with a simple bitwise copy.
    ///
    template <typename TObject>
    concept CTriviallyCopyable = std::is_trivially_copyable_v<TObject>;

} // namespace Concepts

} // namespace Lumen
