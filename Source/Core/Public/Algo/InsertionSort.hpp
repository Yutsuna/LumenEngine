/**
 * @file InsertionSort.hpp
 * @brief Insertion sort algorithm implementation.
 */

#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace Algo
{

    namespace Internal
    {

        /** The threshold value for insertion sort. */
        inline constexpr ISize InsertionSortThreshold = 16;

        /**
         * @brief Unguarded linear insert algorithm for insertion sort.
         * @param InLast The iterator to the last element of the range.
         * @param InPredicate The predicate used to compare elements.
         */
        template <typename TIterator, typename TPredicate>
        void UnguardedLinearInsert ( TIterator InLast, TPredicate &InPredicate ) noexcept;

    } // namespace Internal

    /**
     * @brief Sorts the elements in the range [InFirst, InLast) using the insertion sort algorithm.
     * @param InFirst The iterator to the first element of the range.
     * @param InLast The iterator to the last element of the range.
     * @param InPredicate The predicate used to compare elements.
     */
    template <typename TIterator, typename TPredicate>
    void InsertionSort ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/InsertionSort.inl"
