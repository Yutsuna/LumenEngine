/**
 * @file Quicksort.hpp
 * @brief Quicksort algorithm declaration.
 */

#pragma once

#include <Concepts/Sortable.hpp>
#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace Algo
{

    namespace Internal
    {

        inline constexpr ISize NintherThreshold = 128;

        /** @returns The median of three elements using the given predicate. */
        template <typename TIterator, typename TPredicate>
        TIterator MedianOfThree ( TIterator InFirst, TIterator InSecond, TIterator InThird, TPredicate &InPredicate ) noexcept;

        /** @brief Quicksort loop leaving chunks of elements to be sorted recursively. */
        template <typename TIterator, typename TPredicate>
        void QuickSortLoop ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept;

        /** @brief Moves the median element to the first position in the range. */
        template <typename TIterator, typename TPredicate>
        void MoveMedianToFirst ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept;

        /** @brief Unguarded partition step for quicksort. */
        template <typename TIterator, typename TPredicate>
        [[nodiscard]] TIterator
        UnguardedPartition ( TIterator InLeft, TIterator InRight, TIterator InPivot, TPredicate &InPredicate ) noexcept;

        /** @brief Performs linear insertion sort on the range. */
        template <typename TIterator, typename TPredicate>
        void UnguardedLinearInsert ( TIterator InLast, TPredicate &InPredicate ) noexcept;

    } // namespace Internal

    /**
     * @brief Sorts the range in ascending order <
     * @note Prefer IntroSort unless the input is trusted. Worst case is O(n²).
     */
    template <typename TRangeType>
        requires Concepts::CSortableRange<TRangeType, std::less<>>
    void QuickSort ( TRangeType &&InRange ) noexcept;

    /**
     * @brief Sorts the range in ascending order using the quicksort algorithm.
     * @note Prefer IntroSort unless the input is trusted. Worst case is O(n²).
     */
    template <typename TRangeType, typename TPredicate>
        requires Concepts::CSortableRange<TRangeType, TPredicate>
    void QuickSort ( TRangeType &&InRange ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/QuickSort.inl"
