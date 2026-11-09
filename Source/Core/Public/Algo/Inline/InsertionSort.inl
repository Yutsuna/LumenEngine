/**
 * @file InsertionSort.inl
 * @brief Inline implementation of the insertion sort algorithm.
 */

#pragma once

#include <Algo/InsertionSort.hpp>

#include <algorithm>
#include <iterator>

/**
 * Public
 */

template <typename TIterator, typename TPredicate>
void Lumen::Algo::InsertionSort ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept
{
    using FValue = std::iter_value_t<TIterator>;
    if ( InFirst == InLast )
    {
        return;
    }

    for ( TIterator Current = InFirst + 1; Current != InLast; ++Current )
    {
        if ( InPredicate( *Current, *InFirst ) )
        {
            /** INFO: Shift the whole prefix in one move_backward */
            FValue Value = std::move( *Current );
            std::move_backward( InFirst, Current, Current + 1 );
            *InFirst = std::move( Value );
        }
        else
        {
            Internal::UnguardedLinearInsert( Current, InPredicate );
        }
    }
}

/**
 * Private
 */

template <typename TIterator, typename TPredicate>
void Lumen::Algo::Internal::UnguardedLinearInsert ( TIterator InLast, TPredicate &InPredicate ) noexcept
{
    using FValue = std::iter_value_t<TIterator>;

    FValue Value       = std::move( *InLast );
    TIterator Previous = InLast - 1;

    while ( InPredicate( Value, *Previous ) )
    {
        *InLast = std::move( *Previous );
        InLast  = Previous;
        --Previous;
    }

    *InLast = std::move( Value );
}
