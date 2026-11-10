/**
 * @file AlgoHeapSortTest.cpp
 * @brief Functional tests for Algo::HeapSort.
 */

#include "AlgoSortTestHelper.hpp"

#include <Algo/HeapSort.hpp>
#include <Tests/UnitTest.hpp>

#include <algorithm>
#include <functional>

namespace Lumen
{

namespace Helpers = Tests::SortHelpers;

LUMEN_TEST( Algo_HeapSort, AllShapes )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::HeapSort( Values );
    } );
}

LUMEN_TEST( Algo_HeapSort, SmallSizes )
{
    /** INFO: Covers both parities of the element count: even counts hit the single-child branch of SiftDown. */
    Helpers::CheckSorterOnSmallSizes( [] ( TVector<Int32> &Values )
    {
        Algo::HeapSort( Values );
    } );
}

LUMEN_TEST( Algo_HeapSort, CustomPredicateDescending )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::HeapSort( Values, [] ( const Int32 Left, const Int32 Right ) noexcept
        {
            return Left > Right;
        } );
    }, true );
}

LUMEN_TEST( Algo_HeapSort, NonTrivialValueType )
{
    Helpers::CheckStringSorter( [] ( TVector<FString> &Values )
    {
        Algo::HeapSort( Values );
    } );
}

LUMEN_TEST( Algo_HeapSort, LargeRecordsByKey )
{
    Helpers::CheckRecordSorter( [] ( TVector<Helpers::FSortRecord> &Records )
    {
        Algo::HeapSort( Records, [] ( const Helpers::FSortRecord &Left, const Helpers::FSortRecord &Right ) noexcept
        {
            return Left.Key < Right.Key;
        } );
    } );
}

LUMEN_TEST( Algo_HeapSort, LargeRandomInput )
{
    Helpers::CheckSorterOnShape( [] ( TVector<Int32> &Values )
    {
        Algo::HeapSort( Values );
    }, 100000, Helpers::ESortShape::Random );
}

LUMEN_TEST( Algo_HeapSort, IteratorOverloadSortsOnlyWindow )
{
    TVector<Int32> Values;
    Helpers::FillShape( Values, 400, Helpers::ESortShape::Random, 5ULL );
    const TVector<Int32> Original = Values;

    std::less<> Predicate;
    Algo::HeapSort( Values.begin() + 50, Values.begin() + 250, Predicate );

    LUMEN_CHECK_TRUE( std::is_sorted( Values.begin() + 50, Values.begin() + 250 ) );
    LUMEN_CHECK_TRUE( std::equal( Values.begin(), Values.begin() + 50, Original.begin() ) );
    LUMEN_CHECK_TRUE( std::equal( Values.begin() + 250, Values.end(), Original.begin() + 250 ) );
}

} // namespace Lumen
