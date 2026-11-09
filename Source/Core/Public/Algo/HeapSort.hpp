/**
 * @file HeapSort.hpp
 * @brief Heap sort algorithm declaration.
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
         * @brief Sifts down the element at the given hole index to maintain the heap property.
         *
         * @param InFirst The iterator to the first element of the range.
         * @param InHole The index of the hole to sift down.
         * @param InCount The number of elements in the range.
         * @param InValue The value of the element at the hole index.
         * @param InPredicate The predicate used to compare elements.
         */
        template <typename TIterator, typename TPredicate>
        void SiftDown ( TIterator InFirst,
                        std::iter_difference_t<TIterator> InHole,
                        std::iter_difference_t<TIterator> InCount,
                        std::iter_value_t<TIterator> InValue,
                        TPredicate &InPredicate ) noexcept;

    } // namespace Internal

    /**
     * @brief Sorts the elements in the range [InFirst, InLast) using the heap sort algorithm.
     *
     * @param InFirst The iterator to the first element of the range.
     * @param InLast The iterator to the last element of the range.
     * @param InPredicate The predicate used to compare elements.
     */
    template <typename TIterator, typename TPredicate>
    void HeapSort ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept;

    /**
     * @brief Sorts the range in ascending order (`operator<`) with heap sort.
     *
     * @param InRange The range to sort.
     */
    template <typename TRangeType>
        requires Concepts::CSortableRange<TRangeType, std::less<>>
    void HeapSort ( TRangeType &&InRange ) noexcept;

    /**
     * @brief Sorts the range with heap sort and the given predicate.
     *
     * @param InRange The range to sort.
     * @param InPredicate The predicate used to compare elements.
     */
    template <typename TRangeType, typename TPredicate>
        requires Concepts::CSortableRange<TRangeType, TPredicate>
    void HeapSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/HeapSort.inl"
