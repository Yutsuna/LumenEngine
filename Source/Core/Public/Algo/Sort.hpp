/**
 * @file Sort.hpp
 * @brief Sorting algorithms.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#include <Concepts/Sortable.hpp>

#include <utility>

namespace Lumen
{

namespace Algo
{

    /**
     * @brief Sorts the given range in ascending order <.
     *
     * @tparam TRangeType The type of the range to sort.
     * @tparam TPredicate The type of the predicate to use.
     * @param InRange The range to sort.
     * @param InPredicate The predicate to use.
     *
     * @note O(n log n) worst case
     */
    template <typename TRangeType>
        requires Concepts::CSortableRange<TRangeType, std::less<>>
    LUMEN_FORCE_INLINE void Sort ( TRangeType &&InRange ) noexcept;

    /**
     * @brief Sorts the given range using introsort with the given predicate.
     *
     * @tparam TRangeType The type of the range to sort.
     * @tparam TPredicate The type of the predicate to use.
     * @param Range The range to sort.
     * @param Predicate The predicate to use.
     *
     * @note O(n log n) worst case
     */
    template <typename TRangeType, typename TPredicate>
        requires Concepts::CSortableRange<TRangeType, TPredicate>
    LUMEN_FORCE_INLINE void Sort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/Sort.inl"
