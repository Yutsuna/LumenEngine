/**
 * @file HeapSort.inl
 * @brief Inline implementation of HeapSort algorithm.
 */

#pragma once

#include <Algo/HeapSort.hpp>

#include <Templates/Invoke.hpp>

/**
 * Public
 */

template <typename TIterator, typename TPredicate>
void Lumen::Algo::HeapSort ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept
{
    using FDifference = std::iter_difference_t<TIterator>;
    using FValue      = std::iter_value_t<TIterator>;

    const FDifference Count = InLast - InFirst;
    if ( Count < 2 )
    {
        return;
    }

    /** INFO: Heap construction, from the last internal node up to the root. */
    FDifference Parent = ( Count - 2 ) / 2;
    for ( ;; )
    {
        FValue Value = std::move( *( InFirst + Parent ) );
        Internal::SiftDown( InFirst, Parent, Count, std::move( Value ), InPredicate );

        if ( Parent == 0 )
        {
            break;
        }
        --Parent;
    }

    /** INFO: Pop the maximum to the end, sink the displaced value from the root. */
    for ( FDifference End = Count - 1; End > 0; --End )
    {
        FValue Value       = std::move( *( InFirst + End ) );
        *( InFirst + End ) = std::move( *InFirst );
        Internal::SiftDown( InFirst, 0, End, std::move( Value ), InPredicate );
    }
}

template <typename TRangeType>
    requires Lumen::Concepts::CSortableRange<TRangeType, std::less<>>
void Lumen::Algo::HeapSort ( TRangeType &&InRange ) noexcept
{
    HeapSort( std::forward<TRangeType>( InRange ), std::less<>{} );
}

template <typename TRangeType, typename TPredicate>
    requires Lumen::Concepts::CSortableRange<TRangeType, TPredicate>
void Lumen::Algo::HeapSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept
{
    HeapSort( std::ranges::begin( InRange ), std::ranges::end( InRange ), InPredicate );
}

/**
 * Private
 */

template <typename TIterator, typename TPredicate>
void Lumen::Algo::Internal::SiftDown ( TIterator InFirst,
                                       std::iter_difference_t<TIterator> InHole,
                                       std::iter_difference_t<TIterator> InCount,
                                       std::iter_value_t<TIterator> InValue,
                                       TPredicate &InPredicate ) noexcept
{
    using FDifference = std::iter_difference_t<TIterator>;

    const FDifference Top = InHole;
    FDifference Child     = InHole;

    /** INFO: Descend to a leaf along the larger child, without comparing against InValue. */
    while ( Child < ( InCount - 1 ) / 2 )
    {
        Child = 2 * ( Child + 1 );
        if ( Invoke( InPredicate, *( InFirst + Child ), *( InFirst + ( Child - 1 ) ) ) )
        {
            --Child;
        }

        *( InFirst + InHole ) = std::move( *( InFirst + Child ) );
        InHole                = Child;
    }

    /** INFO: Even count: the last internal node has a single child. */
    if ( ( InCount & 1 ) == 0 and Child == ( InCount - 2 ) / 2 )
    {
        Child                 = ( 2 * Child ) + 1;
        *( InFirst + InHole ) = std::move( *( InFirst + Child ) );
        InHole                = Child;
    }

    /** INFO: Sift InValue back up to its place. */
    FDifference Parent = ( InHole - 1 ) / 2;
    while ( InHole > Top and Invoke( InPredicate, *( InFirst + Parent ), InValue ) )
    {
        *( InFirst + InHole ) = std::move( *( InFirst + Parent ) );
        InHole                = Parent;
        Parent                = ( InHole - 1 ) / 2;
    }

    *( InFirst + InHole ) = std::move( InValue );
}
