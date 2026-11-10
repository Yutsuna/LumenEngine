/**
 * @file QuickSort.hpp
 * @brief QuickSort algorithm declaration.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <Concepts/Sortable.hpp>

namespace Lumen
{

namespace Algo
{

    namespace Internal
    {

        inline constexpr ISize NintherThreshold = 128;

        /** @returns The median of three elements using the given predicate. */
        template <typename TIterator, typename TPredicate>
        [[nodiscard]] TIterator
        MedianOfThree ( TIterator InFirst, TIterator InSecond, TIterator InThird, TPredicate &InPredicate ) noexcept;

        /**
         * @brief Swaps a pivot candidate to InFirst.
         * @pre InLast - InFirst > InsertionSortThreshold.
         */
        template <typename TIterator, typename TPredicate>
        void MoveMedianToFirst ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept;

        /**
         * @brief Hoare partition without bound checks, around the pivot stored at InPivot.
         *
         * @param InLeft First element to scan (InPivot + 1).
         * @param InRight One past the last element to scan.
         * @param InPivot Iterator to the pivot, never moved by the partition.
         *
         * @return Cut such as [first, Cut) <= pivot <= [Cut, last), with first < Cut < last.
         * @pre The pivot was prepared by MoveMedianToFirst (sentinels exist on both sides).
         */
        template <typename TIterator, typename TPredicate>
        [[nodiscard]] TIterator
        UnguardedPartition ( TIterator InLeft, TIterator InRight, TIterator InPivot, TPredicate &InPredicate ) noexcept;

        /**
         * @brief Quicksort loop leaving chunks of at most InsertionSortThreshold elements unsorted.
         */
        template <typename TIterator, typename TPredicate>
        void QuickSortLoop ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept;

    } // namespace Internal

    /**
     * @brief Sorts the range in ascending order (`operator<`) with quicksort.
     *
     * @param InRange The range to sort.
     * @note Not stable. Prefer IntroSort unless the input is trusted: worst case is O(n^2).
     */
    template <typename TRangeType>
        requires Concepts::CSortableRange<TRangeType, std::less<>>
    void QuickSort ( TRangeType &&InRange ) noexcept;

    /**
     * @brief Sorts the range with quicksort and the given predicate.
     *
     * @param InRange The range to sort.
     * @param InPredicate Strict weak ordering returning true when the first argument goes before the second.
     * @note Not stable. Prefer IntroSort unless the input is trusted: worst case is O(n^2).
     */
    template <typename TRangeType, typename TPredicate>
        requires Concepts::CSortableRange<TRangeType, TPredicate>
    void QuickSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/QuickSort.inl"
