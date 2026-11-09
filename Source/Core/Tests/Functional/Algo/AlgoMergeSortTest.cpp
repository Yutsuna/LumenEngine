/**
 * @file AlgoMergeSortTest.cpp
 * @brief Functional tests for Algo::MergeSort.
 */

#include "AlgoSortTestHelper.hpp"

#include <Algo/MergeSort.hpp>
#include <Tests/UnitTest.hpp>

#include <algorithm>
#include <functional>

namespace Lumen
{

namespace Helpers = Tests::SortHelpers;

LUMEN_TEST( Algo_MergeSort, AllShapes )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::MergeSort( Values );
    } );
}

LUMEN_TEST( Algo_MergeSort, SmallSizes )
{
    /** INFO: Covers the recursion-base cutoff and the "already ordered halves" fast path around InsertionSortThreshold. */
    Helpers::CheckSorterOnSmallSizes( [] ( TVector<Int32> &Values )
    {
        Algo::MergeSort( Values );
    } );
}

LUMEN_TEST( Algo_MergeSort, CustomPredicateDescending )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::MergeSort( Values, [] ( const Int32 Left, const Int32 Right ) noexcept
        {
            return Left > Right;
        } );
    }, true );
}

LUMEN_TEST( Algo_MergeSort, NonTrivialValueType )
{
    Helpers::CheckStringSorter( [] ( TVector<FString> &Values )
    {
        Algo::MergeSort( Values );
    } );
}

LUMEN_TEST( Algo_MergeSort, LargeRecordsByKey )
{
    Helpers::CheckRecordSorter( [] ( TVector<Helpers::FSortRecord> &Records )
    {
        Algo::MergeSort( Records, [] ( const Helpers::FSortRecord &Left, const Helpers::FSortRecord &Right ) noexcept
        {
            return Left.Key < Right.Key;
        } );
    } );
}

LUMEN_TEST( Algo_MergeSort, IsStable )
{
    TVector<Helpers::FSortRecord> Records;
    Helpers::FillRecords( Records, 1500, 20ULL, 7ULL );

    Algo::MergeSort( Records, [] ( const Helpers::FSortRecord &Left, const Helpers::FSortRecord &Right ) noexcept
    {
        return Left.Key < Right.Key;
    } );

    /** INFO: Extra is the original index: equal keys must keep their original order. */
    for ( USize Index = 1; Index < Records.size(); ++Index )
    {
        const Helpers::FSortRecord &Previous = Records[Index - 1];
        const Helpers::FSortRecord &Current  = Records[Index];

        LUMEN_CHECK_TRUE( Previous.Key < Current.Key or ( Previous.Key == Current.Key and Previous.Extra < Current.Extra ) );
    }
}

LUMEN_TEST( Algo_MergeSort, LargeInputs )
{
    for ( const Helpers::ESortShape Shape : { Helpers::ESortShape::Random, Helpers::ESortShape::OrganPipe,
                                              Helpers::ESortShape::SawTooth, Helpers::ESortShape::FewUnique } )
    {
        Helpers::CheckSorterOnShape( [] ( TVector<Int32> &Values )
        {
            Algo::MergeSort( Values );
        }, 200000, Shape );
    }
}

LUMEN_TEST( Algo_MergeSort, IteratorOverloadSortsOnlyWindow )
{
    TVector<Int32> Values;
    Helpers::FillShape( Values, 400, Helpers::ESortShape::Random, 5ULL );
    const TVector<Int32> Original = Values;

    std::less<> Predicate;
    Algo::MergeSort( Values.begin() + 50, Values.begin() + 250, Predicate );

    LUMEN_CHECK_TRUE( std::is_sorted( Values.begin() + 50, Values.begin() + 250 ) );
    LUMEN_CHECK_TRUE( std::equal( Values.begin(), Values.begin() + 50, Original.begin() ) );
    LUMEN_CHECK_TRUE( std::equal( Values.begin() + 250, Values.end(), Original.begin() + 250 ) );
}

} // namespace Lumen
