/**
 * @file Sort.hpp
 * @brief Default sorting entry points.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#include <Concepts/Sortable.hpp>

#include <Algo/IntroSort.hpp>

namespace Lumen
{

namespace Algo
{

    /**
     * @brief Sorts the range in ascending order (`operator<`) using introsort.
     *
     * @param InRange The range to sort.
     * @note Not stable. O(n log n) worst case, in place, no allocation.
     */
    template <typename TRangeType>
        requires Concepts::CSortableRange<TRangeType, std::less<>>
    LUMEN_FORCE_INLINE void Sort ( TRangeType &&InRange ) noexcept;

    /**
     * @brief Sorts the range using introsort and the given predicate.
     *
     * @param InRange The range to sort.
     * @param InPredicate The predicate to use for comparison.
     * @note Not stable. O(n log n) worst case, in place, no allocation.
     */
    template <typename TRangeType, typename TPredicate>
        requires Concepts::CSortableRange<TRangeType, TPredicate>
    LUMEN_FORCE_INLINE void Sort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/Sort.inl"
