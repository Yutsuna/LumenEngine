/**
 * @file AlgoQuickSortTest.cpp
 * @brief Functional tests for Algo::QuickSort and its partitioning primitives.
 */

#include "AlgoSortTestHelper.hpp"

#include <Algo/QuickSort.hpp>
#include <Tests/UnitTest.hpp>

#include <algorithm>
#include <functional>

namespace Lumen
{

namespace Helpers = Tests::SortHelpers;

/**
 * Public
 */

LUMEN_TEST( Algo_QuickSort, AllShapes )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::QuickSort( Values );
    } );
}

LUMEN_TEST( Algo_QuickSort, SmallSizes )
{
    Helpers::CheckSorterOnSmallSizes( [] ( TVector<Int32> &Values )
    {
        Algo::QuickSort( Values );
    } );
}

LUMEN_TEST( Algo_QuickSort, CustomPredicateDescending )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::QuickSort( Values, [] ( const Int32 Left, const Int32 Right ) noexcept
        {
            return Left > Right;
        } );
    }, true );
}

LUMEN_TEST( Algo_QuickSort, NonTrivialValueType )
{
    Helpers::CheckStringSorter( [] ( TVector<FString> &Values )
    {
        Algo::QuickSort( Values );
    } );
}

LUMEN_TEST( Algo_QuickSort, LargeRecordsByKey )
{
    Helpers::CheckRecordSorter( [] ( TVector<Helpers::FSortRecord> &Records )
    {
        Algo::QuickSort( Records, [] ( const Helpers::FSortRecord &Left, const Helpers::FSortRecord &Right ) noexcept
        {
            return Left.Key < Right.Key;
        } );
    } );
}

/**
 * Internals
 */

LUMEN_TEST( Algo_QuickSort, MedianOfThreeAllPermutations )
{
    const auto CheckPermutations = [] ( TArray<Int32, 3> Candidate )
    {
        /** INFO: Candidate is given in ascending order, so next_permutation walks every permutation. */
        const Int32 ExpectedMedian = Candidate[1];
        std::less<> Predicate;

        do
        {
            const auto Median =
                Algo::Internal::MedianOfThree( Candidate.begin(), Candidate.begin() + 1, Candidate.begin() + 2, Predicate );
            LUMEN_CHECK_EQ( *Median, ExpectedMedian );
        } while ( std::ranges::next_permutation( Candidate ).found );
    };

    CheckPermutations( { 1, 2, 3 } );
    CheckPermutations( { 1, 1, 2 } );
    CheckPermutations( { 1, 2, 2 } );
    CheckPermutations( { 5, 5, 5 } );
}

LUMEN_TEST( Algo_QuickSort, PartitionInvariants )
{
    std::less<> Predicate;

    for ( const USize Count : Helpers::SortTestSizes )
    {
        if ( Count <= static_cast<USize>( Algo::Internal::InsertionSortThreshold ) )
        {
            continue;
        }

        for ( const Helpers::ESortShape Shape : Helpers::AllShapes )
        {
            TVector<Int32> Values;
            Helpers::FillShape( Values, Count, Shape, 77ULL );

            Algo::Internal::MoveMedianToFirst( Values.begin(), Values.end(), Predicate );
            const Int32 Pivot = Values.front();
            const auto Cut    = Algo::Internal::UnguardedPartition( Values.begin() + 1, Values.end(), Values.begin(), Predicate );

            LUMEN_CHECK_TRUE( Cut > Values.begin() );
            LUMEN_CHECK_TRUE( Cut < Values.end() );
            LUMEN_CHECK_EQ( Values.front(), Pivot );
            LUMEN_CHECK_TRUE( std::none_of( Values.begin(), Cut, [Pivot] ( const Int32 Value )
            {
                return Pivot < Value;
            } ) );
            LUMEN_CHECK_TRUE( std::none_of( Cut, Values.end(), [Pivot] ( const Int32 Value )
            {
                return Value < Pivot;
            } ) );
        }
    }
}

} // namespace Lumen
