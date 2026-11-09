/**
 * @file AlgoIntroSortTest.cpp
 * @brief Functional tests for Algo::IntroSort and its heapsort fallback.
 */

#include "AlgoSortTestHelper.hpp"

#include <Algo/InsertionSort.hpp>
#include <Algo/IntroSort.hpp>
#include <Tests/UnitTest.hpp>

#include <algorithm>
#include <format>
#include <functional>
#include <initializer_list>
#include <utility>

namespace Lumen
{

namespace Helpers = Tests::SortHelpers;

LUMEN_TEST( Algo_IntroSort, AllShapes )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::IntroSort( Values );
    } );
}

LUMEN_TEST( Algo_IntroSort, SmallSizes )
{
    Helpers::CheckSorterOnSmallSizes( [] ( TVector<Int32> &Values )
    {
        Algo::IntroSort( Values );
    } );
}

LUMEN_TEST( Algo_IntroSort, CustomPredicateDescending )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::IntroSort( Values, [] ( const Int32 Left, const Int32 Right ) noexcept
        {
            return Left > Right;
        } );
    }, true );
}

LUMEN_TEST( Algo_IntroSort, NonTrivialValueType )
{
    Helpers::CheckStringSorter( [] ( TVector<FString> &Values )
    {
        Algo::IntroSort( Values );
    } );
}

LUMEN_TEST( Algo_IntroSort, LargeRecordsByKey )
{
    Helpers::CheckRecordSorter( [] ( TVector<Helpers::FSortRecord> &Records )
    {
        Algo::IntroSort( Records, [] ( const Helpers::FSortRecord &Left, const Helpers::FSortRecord &Right ) noexcept
        {
            return Left.Key < Right.Key;
        } );
    } );
}

LUMEN_TEST( Algo_IntroSort, LargeInputs )
{
    for ( const Helpers::ESortShape Shape : { Helpers::ESortShape::Random, Helpers::ESortShape::OrganPipe,
                                              Helpers::ESortShape::SawTooth } )
    {
        Helpers::CheckSorterOnShape( [] ( TVector<Int32> &Values )
        {
            Algo::IntroSort( Values );
        }, 200000, Shape );
    }
}

LUMEN_TEST( Algo_IntroSort, DepthLimitFallsBackToHeapSort )
{
    /** INFO: A tiny depth budget forces the heapsort fallback on large sub-ranges. */
    for ( const Int32 DepthLimit : { 0, 1, 2, 3 } )
    {
        for ( const Helpers::ESortShape Shape : Helpers::AllShapes )
        {
            TVector<Int32> Values;
            Helpers::FillShape( Values, 5000, Shape, 1234ULL );

            TVector<Int32> Expected = Values;
            std::ranges::sort( Expected );

            std::less<> Predicate;
            Algo::Internal::IntroSortLoop( Values.begin(), Values.end(), DepthLimit, Predicate );
            Algo::InsertionSort( Values.begin(), Values.end(), Predicate );

            LUMEN_CHECK_MSG( Values == Expected,
                             std::format( "DepthLimit={} Shape={}", DepthLimit, std::to_underlying( Shape ) ) );
        }
    }
}

} // namespace Lumen
