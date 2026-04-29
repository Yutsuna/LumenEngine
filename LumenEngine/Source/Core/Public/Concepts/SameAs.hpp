/**
 * @file SameAs.hpp
 * @brief Concepts for type equivalence in LumenEngine
 */

#pragma once

#include <concepts>

namespace LumenEngine
{

namespace Concepts
{

    /**
     * @concept CSameAs
     * @brief Checks if two types are the same, ignoring const/volatile qualifiers and references.
     * @tparam Actual The type to check.
     * @tparam Expected The type to compare against.
     */
    template <typename Actual, typename Expected>
    concept CSameAs = std::same_as<Actual, Expected>;

} // namespace Concepts

} // namespace LumenEngine
