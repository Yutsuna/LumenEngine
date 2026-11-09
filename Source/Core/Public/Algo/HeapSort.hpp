/**
 * @file HeapSort.hpp
 * @brief Heap sort algorithm implementation.
 */

#pragma once

namespace Lumen
{

namespace Algo
{

    /**
     * @brief Sorts the elements in the range [InFirst, InLast) using the heap sort algorithm.
     *
     * @param InFirst The iterator to the first element of the range.
     * @param InLast The iterator to the last element of the range.
     * @param InPredicate The predicate used to compare elements.
     */
    template <typename TIterator, typename TPredicate>
    void HeapSort ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/HeapSort.inl"
