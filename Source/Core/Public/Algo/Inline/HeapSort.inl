/**
 * @file HeapSort.inl
 * @brief Inline implementation of HeapSort algorithm.
 */

#pragma once

#include <Algo/HeapSort.hpp>

#include <algorithm>

template <typename TIterator, typename TPredicate>
void Lumen::Algo::HeapSortFallback ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept
{
    /** INFO: Cold path, the standard heap routines are well tuned and not worth duplicating. */
    std::make_heap( InFirst, InLast, InPredicate );
    std::sort_heap( InFirst, InLast, InPredicate );
}
