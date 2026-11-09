/**
 * @file Sortable.hpp
 * @brief Concept for a sortable range.
 */

#pragma once

#include <ranges>

namespace Lumen
{

namespace Concepts
{

    /**
     * @concept CSortableRange
     * @brief Concept for a sortable range.
     * @tparam TRangeType The type of the range.
     * @tparam TPredicate The type of the predicate.
     */
    template <typename TRangeType, typename TPredicate>
    concept CSortableRange = std::ranges::random_access_range<TRangeType> and std::ranges::common_range<TRangeType> and
                             std::sortable<std::ranges::iterator_t<TRangeType>, TPredicate>;

} // namespace Concepts

} // namespace Lumen
