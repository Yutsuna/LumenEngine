/**
 * @file QuickSort.inl
 * @brief Inline implementation of QuickSort and its partitioning primitives.
 */

#pragma once

#include <Algo/InsertionSort.hpp>
#include <Algo/QuickSort.hpp>

#include <Concepts/TriviallyCopyable.hpp>

/**
 * Public
 */

template <typename TRangeType>
    requires Lumen::Concepts::CSortableRange<TRangeType, std::less<>>
void Lumen::Algo::QuickSort ( TRangeType &&InRange ) noexcept
{
    QuickSort( std::forward<TRangeType>( InRange ), std::less<>{} );
}

template <typename TRangeType, typename TPredicate>
    requires Lumen::Concepts::CSortableRange<TRangeType, TPredicate>
void Lumen::Algo::QuickSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept
{
    const auto First = std::ranges::begin( InRange );
    const auto Last  = std::ranges::end( InRange );

    if ( Last - First < 2 )
    {
        return;
    }

    Internal::QuickSortLoop( First, Last, InPredicate );
    InsertionSort( First, Last, InPredicate );
}

/**
 * Internal
 */

template <typename TIterator, typename TPredicate>
void Lumen::Algo::Internal::QuickSortLoop ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept
{
    using FDifference               = std::iter_difference_t<TIterator>;
    constexpr FDifference Threshold = static_cast<FDifference>( InsertionSortThreshold );

    while ( InLast - InFirst > Threshold )
    {
        MoveMedianToFirst( InFirst, InLast, InPredicate );
        const TIterator Cut = UnguardedPartition( InFirst + 1, InLast, InFirst, InPredicate );

        if ( Cut - InFirst < InLast - Cut )
        {
            QuickSortLoop( InFirst, Cut, InPredicate );
            InFirst = Cut;
        }
        else
        {
            QuickSortLoop( Cut, InLast, InPredicate );
            InLast = Cut;
        }
    }
}

template <typename TIterator, typename TPredicate>
void Lumen::Algo::Internal::MoveMedianToFirst ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept
{
    using FDifference       = std::iter_difference_t<TIterator>;
    const FDifference Count = InLast - InFirst;
    const TIterator Low     = InFirst + 1;
    const TIterator High    = InLast - 1;
    const TIterator Mid     = Low + ( Count / 2 );

    TIterator Pivot = Mid;

    if ( Count > static_cast<FDifference>( NintherThreshold ) )
    {
        /** INFO: Tukey's ninther, median of three medians. */
        const FDifference Step = Count / 8;
        Pivot                  = MedianOfThree( MedianOfThree( Low, Low + Step, Low + ( Step * 2 ), InPredicate ),
                                                MedianOfThree( Mid - Step, Mid, Mid + Step, InPredicate ),
                                                MedianOfThree( High - ( Step * 2 ), High - Step, High, InPredicate ), InPredicate );
    }
    else
    {
        Pivot = MedianOfThree( Low, Mid, High, InPredicate );
    }

    std::iter_swap( InFirst, Pivot );
}

template <typename TIterator, typename TPredicate>
TIterator Lumen::Algo::Internal::MedianOfThree ( TIterator InFirst,
                                                 TIterator InSecond,
                                                 TIterator InThird,
                                                 TPredicate &InPredicate ) noexcept
{
    if ( InPredicate( *InFirst, *InSecond ) )
    {
        if ( InPredicate( *InSecond, *InThird ) )
        {
            return InSecond;
        }
        return InPredicate( *InFirst, *InThird ) ? InThird : InFirst;
    }

    if ( InPredicate( *InFirst, *InThird ) )
    {
        return InFirst;
    }
    return InPredicate( *InSecond, *InThird ) ? InThird : InSecond;
}

template <typename TIterator, typename TPredicate>
TIterator Lumen::Algo::Internal::UnguardedPartition ( TIterator InLeft,
                                                      TIterator InRight,
                                                      TIterator InPivot,
                                                      TPredicate &InPredicate ) noexcept
{
    using FValue = std::iter_value_t<TIterator>;

    /**
     * INFO: Heavy types are read by reference:
     *       the pivot slot is never swapped, so the reference stays valid.
     */
    constexpr Bool bCopyPivot = Concepts::CTriviallyCopyable<FValue> and ( sizeof( FValue ) <= ( 2U * sizeof( void * ) ) );
    std::conditional_t<bCopyPivot, const FValue, const FValue &> PivotValue = *InPivot;

    for ( ;; )
    {
        while ( InPredicate( *InLeft, PivotValue ) )
        {
            ++InLeft;
        }

        --InRight;
        while ( InPredicate( PivotValue, *InRight ) )
        {
            --InRight;
        }

        if ( not( InLeft < InRight ) )
        {
            return InLeft;
        }

        std::iter_swap( InLeft, InRight );
        ++InLeft;
    }
}
