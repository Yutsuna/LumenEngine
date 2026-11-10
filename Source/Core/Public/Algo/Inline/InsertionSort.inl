/**
 * @file InsertionSort.inl
 * @brief Inline implementation of the insertion sort algorithm.
 */

#pragma once

#include <Algo/InsertionSort.hpp>
#include <Templates/Invoke.hpp>

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
            /** INFO: *InFirst is not greater than *Current, it acts as the sentinel. */
            Internal::UnguardedLinearInsert( Current, InPredicate );
        }
    }
}

template <typename TRangeType>
    requires Lumen::Concepts::CSortableRange<TRangeType, std::less<>>
void Lumen::Algo::InsertionSort ( TRangeType &&InRange ) noexcept
{
    InsertionSort( std::forward<TRangeType>( InRange ), std::less<>{} );
}

template <typename TRangeType, typename TPredicate>
    requires Lumen::Concepts::CSortableRange<TRangeType, TPredicate>
void Lumen::Algo::InsertionSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept
{
    InsertionSort( std::ranges::begin( InRange ), std::ranges::end( InRange ), InPredicate );
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

    while ( Invoke( InPredicate, Value, *Previous ) )
    {
        *InLast = std::move( *Previous );
        InLast  = Previous;
        --Previous;
    }

    *InLast = std::move( Value );
}
