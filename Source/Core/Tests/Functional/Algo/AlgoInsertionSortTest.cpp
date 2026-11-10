/**
 * @file AlgoInsertionSortTest.cpp
 * @brief Functional tests for Algo::InsertionSort.
 */

#include "AlgoSortTestHelper.hpp"

#include <Algo/InsertionSort.hpp>
#include <Tests/UnitTest.hpp>

#include <algorithm>
#include <functional>

namespace Lumen
{

namespace Helpers = Tests::SortHelpers;

LUMEN_TEST( Algo_InsertionSort, AllShapes )
{
    /** INFO: O(n^2): capped at 1000 elements. */
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::InsertionSort( Values );
    }, false, 1000 );
}

LUMEN_TEST( Algo_InsertionSort, SmallSizes )
{
    Helpers::CheckSorterOnSmallSizes( [] ( TVector<Int32> &Values )
    {
        Algo::InsertionSort( Values );
    } );
}

LUMEN_TEST( Algo_InsertionSort, CustomPredicateDescending )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::InsertionSort( Values, [] ( const Int32 Left, const Int32 Right ) noexcept
        {
            return Left > Right;
        } );
    }, true, 1000 );
}

LUMEN_TEST( Algo_InsertionSort, NonTrivialValueType )
{
    Helpers::CheckStringSorter( [] ( TVector<FString> &Values )
    {
        Algo::InsertionSort( Values );
    } );
}

LUMEN_TEST( Algo_InsertionSort, RecordsByKey )
{
    Helpers::CheckRecordSorter( [] ( TVector<Helpers::FSortRecord> &Records )
    {
        Algo::InsertionSort( Records, [] ( const Helpers::FSortRecord &Left, const Helpers::FSortRecord &Right ) noexcept
        {
            return Left.Key < Right.Key;
        } );
    } );
}

LUMEN_TEST( Algo_InsertionSort, IsStable )
{
    TVector<Helpers::FSortRecord> Records;
    Helpers::FillRecords( Records, 1500, 20ULL, 7ULL );

    Algo::InsertionSort( Records, [] ( const Helpers::FSortRecord &Left, const Helpers::FSortRecord &Right ) noexcept
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

LUMEN_TEST( Algo_InsertionSort, IteratorOverloadSortsOnlyWindow )
{
    TVector<Int32> Values;
    Helpers::FillShape( Values, 400, Helpers::ESortShape::Random, 5ULL );
    const TVector<Int32> Original = Values;

    std::less<> Predicate;
    Algo::InsertionSort( Values.begin() + 50, Values.begin() + 250, Predicate );

    LUMEN_CHECK_TRUE( std::is_sorted( Values.begin() + 50, Values.begin() + 250 ) );
    LUMEN_CHECK_TRUE( std::equal( Values.begin(), Values.begin() + 50, Original.begin() ) );
    LUMEN_CHECK_TRUE( std::equal( Values.begin() + 250, Values.end(), Original.begin() + 250 ) );
}

} // namespace Lumen
