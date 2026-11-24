/**
 * @file Sortable.hpp
 * @brief Concept for a sortable range.
 */

#pragma once

#include <Algo/Range.hpp>

#include <iterator>

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
    concept CSortableRange =
        CRandomAccessRange<TRangeType> and std::sortable<Algo::Internal::TRangeIteratorT<TRangeType>, TPredicate>;

} // namespace Concepts

} // namespace Lumen
