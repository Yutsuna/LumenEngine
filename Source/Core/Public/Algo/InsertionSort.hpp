/**
 * @file InsertionSort.hpp
 * @brief Insertion sort algorithm declaration.
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

        /** @brief Threshold for insertion sort algorithm. */
        inline constexpr ISize InsertionSortThreshold = 16;

        /**
         * @brief Unguarded linear insert algorithm for insertion sort.
         *
         * @param InLast The iterator pointing to the last element of the range.
         * @param InPredicate The predicate used to compare elements.
         */
        template <typename TIterator, typename TPredicate>
        void UnguardedLinearInsert ( TIterator InLast, TPredicate &InPredicate ) noexcept;

    } // namespace Internal

    /**
     * @brief Insertion sort algorithm.
     *
     * @param InFirst The iterator pointing to the first element of the range.
     * @param InLast The iterator pointing to the last element of the range.
     * @param InPredicate The predicate used to compare elements.
     */
    template <typename TIterator, typename TPredicate>
    void InsertionSort ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept;

    /**
     * @brief Insertion sort algorithm for sortable ranges.
     *
     * @param InRange The range to be sorted.
     */
    template <typename TRangeType>
        requires Concepts::CSortableRange<TRangeType, std::less<>>
    void InsertionSort ( TRangeType &&InRange ) noexcept;

    /**
     * @brief Insertion sort algorithm for sortable ranges with a custom predicate.
     *
     * @param InRange The range to be sorted.
     * @param InPredicate The predicate used to compare elements.
     */
    template <typename TRangeType, typename TPredicate>
        requires Concepts::CSortableRange<TRangeType, TPredicate>
    void InsertionSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/InsertionSort.inl"
