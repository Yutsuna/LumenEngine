/**
 * @file MergeSort.hpp
 * @brief Stable merge sort algorithm declaration.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <Concepts/Sortable.hpp>

#include <iterator>

namespace Lumen
{

namespace Algo
{

    namespace Internal
    {

        /**
         * @brief Recursively splits the range, sorts each half and merges them back.
         *
         * @param InFirst The iterator to the first element of the range.
         * @param InLast The iterator past the last element of the range.
         * @param InBuffer The auxiliary buffer used for merging.
         * @param InPredicate The predicate used to compare elements.
         */
        template <typename TIterator, typename TPredicate>
        void MergeSortRecurse ( TIterator InFirst,
                                TIterator InLast,
                                std::iter_value_t<TIterator> *InBuffer,
                                TPredicate &InPredicate ) noexcept;

        /**
         * @brief Merges two adjacent sorted runs back into the source range.
         *
         * @param InFirst The iterator to the first element of the left run.
         * @param InMiddle The iterator to the first element of the right run.
         * @param InLast The iterator past the last element of the right run.
         * @param InBuffer The auxiliary buffer used for merging.
         * @param InPredicate The predicate used to compare elements.
         */
        template <typename TIterator, typename TPredicate>
        void MergeRuns ( TIterator InFirst,
                         TIterator InMiddle,
                         TIterator InLast,
                         std::iter_value_t<TIterator> *InBuffer,
                         TPredicate &InPredicate ) noexcept;

    } // namespace Internal

    /**
     * @brief Sorts the elements in the range [InFirst, InLast) using a stable merge sort.
     *
     * @param InFirst The iterator to the first element of the range.
     * @param InLast The iterator past the last element of the range.
     * @param InPredicate The predicate used to compare elements.
     * @note Stable. O(n log n) worst case, O(n) auxiliary memory.
     */
    template <typename TIterator, typename TPredicate>
    void MergeSort ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept;

    /**
     * @brief Sorts the range in ascending order (`operator<`) with a stable merge sort.
     *
     * @param InRange The range to sort.
     * @note Stable. O(n log n) worst case, O(n) auxiliary memory.
     */
    template <typename TRangeType>
        requires Concepts::CSortableRange<TRangeType, std::less<>>
    void MergeSort ( TRangeType &&InRange ) noexcept;

    /**
     * @brief Sorts the range with a stable merge sort and the given predicate.
     *
     * @param InRange The range to sort.
     * @param InPredicate The predicate used to compare elements.
     * @note Stable. O(n log n) worst case, O(n) auxiliary memory.
     */
    template <typename TRangeType, typename TPredicate>
        requires Concepts::CSortableRange<TRangeType, TPredicate>
    void MergeSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/MergeSort.inl"
