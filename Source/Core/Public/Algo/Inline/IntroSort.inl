/**
 * @file IntroSort.inl
 * @brief Inline implementation of IntroSort.
 */

#pragma once

#include <Algo/HeapSort.hpp>
#include <Algo/InsertionSort.hpp>
#include <Algo/IntroSort.hpp>
#include <Algo/QuickSort.hpp>

/**
 * Public
 */

template <typename TRangeType>
    requires Lumen::Concepts::CSortableRange<TRangeType, std::less<>>
void Lumen::Algo::IntroSort ( TRangeType &&InRange ) noexcept
{
    IntroSort( std::forward<TRangeType>( InRange ), std::less<>{} );
}

template <typename TRangeType, typename TPredicate>
    requires Lumen::Concepts::CSortableRange<TRangeType, TPredicate>
void Lumen::Algo::IntroSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept
{
    const auto First = std::ranges::begin( InRange );
    const auto Last  = std::ranges::end( InRange );

    const auto Count = Last - First;
    if ( Count < 2 )
    {
        return;
    }

    const Int32 DepthLimit = ( static_cast<Int32>( std::bit_width( static_cast<USize>( Count ) ) ) - 1 ) * 2;

    Internal::IntroSortLoop( First, Last, DepthLimit, InPredicate );
    InsertionSort( First, Last, InPredicate );
}

/**
 * Private
 */

template <typename TIterator, typename TPredicate>
void Lumen::Algo::Internal::IntroSortLoop ( TIterator InFirst,
                                            TIterator InLast,
                                            Int32 InDepthLimit,
                                            TPredicate &InPredicate ) noexcept
{
    using FDifference               = std::iter_difference_t<TIterator>;
    constexpr FDifference Threshold = static_cast<FDifference>( InsertionSortThreshold );

    while ( InLast - InFirst > Threshold )
    {
        if ( InDepthLimit == 0 ) [[unlikely]]
        {
            /** INFO: The range is fully sorted afterwards, the final insertion pass is a no-op on it. */
            HeapSort( InFirst, InLast, InPredicate );
            return;
        }
        --InDepthLimit;

        MoveMedianToFirst( InFirst, InLast, InPredicate );
        const TIterator Cut = UnguardedPartition( InFirst + 1, InLast, InFirst, InPredicate );

        if ( Cut - InFirst < InLast - Cut )
        {
            IntroSortLoop( InFirst, Cut, InDepthLimit, InPredicate );
            InFirst = Cut;
        }
        else
        {
            IntroSortLoop( Cut, InLast, InDepthLimit, InPredicate );
            InLast = Cut;
        }
    }
}
