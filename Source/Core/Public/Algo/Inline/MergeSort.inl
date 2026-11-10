/**
 * @file MergeSort.inl
 * @brief Inline implementation of the stable MergeSort algorithm.
 */

#pragma once

#include <Algo/InsertionSort.hpp>
#include <Algo/MergeSort.hpp>

#include <Templates/Invoke.hpp>
#include <Templates/Vector.hpp>

/**
 * Public
 */

template <typename TIterator, typename TPredicate>
void Lumen::Algo::MergeSort ( TIterator InFirst, TIterator InLast, TPredicate &InPredicate ) noexcept
{
    using FDifference = std::iter_difference_t<TIterator>;
    using FValue      = std::iter_value_t<TIterator>;

    const FDifference Count = InLast - InFirst;
    if ( Count < 2 )
    {
        return;
    }

    /** INFO: Single allocation for the auxiliary buffer, reused by every merge pass. */
    TVector<FValue> Buffer;
    Buffer.resize( static_cast<USize>( Count ) );

    Internal::MergeSortRecurse( InFirst, InLast, Buffer.data(), InPredicate );
}

template <typename TRangeType>
    requires Lumen::Concepts::CSortableRange<TRangeType, std::less<>>
void Lumen::Algo::MergeSort ( TRangeType &&InRange ) noexcept
{
    MergeSort( std::forward<TRangeType>( InRange ), std::less<>{} );
}

template <typename TRangeType, typename TPredicate>
    requires Lumen::Concepts::CSortableRange<TRangeType, TPredicate>
void Lumen::Algo::MergeSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept
{
    MergeSort( std::ranges::begin( InRange ), std::ranges::end( InRange ), InPredicate );
}

/**
 * Private
 */

template <typename TIterator, typename TPredicate>
void Lumen::Algo::Internal::MergeSortRecurse ( TIterator InFirst,
                                               TIterator InLast,
                                               std::iter_value_t<TIterator> *InBuffer,
                                               TPredicate &InPredicate ) noexcept
{
    using FDifference               = std::iter_difference_t<TIterator>;
    constexpr FDifference Threshold = static_cast<FDifference>( InsertionSortThreshold );

    const FDifference Count = InLast - InFirst;
    if ( Count <= Threshold )
    {
        /** INFO: Small chunks finish with insertion sort — stable and cache-friendly. */
        InsertionSort( InFirst, InLast, InPredicate );
        return;
    }

    const TIterator Middle = InFirst + ( Count / 2 );
    MergeSortRecurse( InFirst, Middle, InBuffer, InPredicate );
    MergeSortRecurse( Middle, InLast, InBuffer, InPredicate );

    /** INFO: Skip the merge when the two halves are already in order. */
    if ( not Invoke( InPredicate, *Middle, *( Middle - 1 ) ) )
    {
        return;
    }

    MergeRuns( InFirst, Middle, InLast, InBuffer, InPredicate );
}

template <typename TIterator, typename TPredicate>
void Lumen::Algo::Internal::MergeRuns ( TIterator InFirst,
                                        TIterator InMiddle,
                                        TIterator InLast,
                                        std::iter_value_t<TIterator> *InBuffer,
                                        TPredicate &InPredicate ) noexcept
{
    using FValue = std::iter_value_t<TIterator>;

    TIterator Left  = InFirst;
    TIterator Right = InMiddle;
    FValue *Out     = InBuffer;

    /** INFO: Stable merge: on equal keys, the left run wins. */
    while ( Left != InMiddle and Right != InLast )
    {
        if ( Invoke( InPredicate, *Right, *Left ) )
        {
            *Out = std::move( *Right );
            ++Right;
        }
        else
        {
            *Out = std::move( *Left );
            ++Left;
        }
        ++Out;
    }

    Out = std::move( Left, InMiddle, Out );
    Out = std::move( Right, InLast, Out );

    /** INFO: Copy the merged run back into the source range. */
    std::move( InBuffer, Out, InFirst );
}
