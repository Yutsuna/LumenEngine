/**
 * @file IntroSort.hpp
 * @brief Introspective sort declaration: quicksort with a heapsort fallback.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <Concepts/Sortable.hpp>

#include <Algo/QuickSort.hpp>

namespace Lumen
{

namespace Algo
{

    namespace Internal
    {

        /**
         * @brief Quicksort loop switching to HeapSort once InDepthLimit partitions deep.
         * @param InFirst Iterator to the first element of the sequence.
         * @param InLast Iterator past the last element of the sequence.
         * @param InDepthLimit Remaining partition budget.
         * @param InPredicate Predicate used to compare elements.
         * @details Leaves chunks of at most InsertionSortThreshold elements unsorted: the caller finishes with
         *          InsertionSort.
         */
        template <typename TIterator, typename TPredicate>
        void IntroSortLoop ( TIterator InFirst, TIterator InLast, Int32 InDepthLimit, TPredicate &InPredicate ) noexcept;

    } // namespace Internal

    /**
     * @brief Sorts the range in ascending order (`operator<`) with introsort.
     * @param InRange The range to sort.
     * @note Not stable. O(n log n) worst case, in place, no allocation.
     */
    template <typename TRangeType>
        requires Concepts::CSortableRange<TRangeType, std::less<>>
    void IntroSort ( TRangeType &&InRange ) noexcept;

    /**
     * @brief Sorts the range with introsort and the given predicate.
     * @param InRange The range to sort.
     * @param InPredicate Strict weak ordering returning true when the first argument goes before the second.
     * @details Depth budget of 2 * floor( log2( n ) ) partitions, then HeapSort on the offending sub-range.
     * @note Not stable. O(n log n) worst case, in place, no allocation.
     */
    template <typename TRangeType, typename TPredicate>
        requires Concepts::CSortableRange<TRangeType, TPredicate>
    void IntroSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/IntroSort.inl"
