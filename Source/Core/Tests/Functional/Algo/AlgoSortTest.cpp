/**
 * @file AlgoSortTest.cpp
 * @brief Functional tests for the Algo::Sort entry points.
 */

#include "AlgoSortTestHelper.hpp"

#include <Algo/Sort.hpp>

#include <Memory/Span.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

#include <algorithm>

namespace Lumen
{

namespace Helpers = Tests::SortHelpers;

LUMEN_TEST( Algo_Sort, AllShapes )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::Sort( Values );
    } );
}

LUMEN_TEST( Algo_Sort, CustomPredicateDescending )
{
    Helpers::CheckSorterOnAllShapes( [] ( TVector<Int32> &Values )
    {
        Algo::Sort( Values, [] ( const Int32 Left, const Int32 Right ) noexcept
        {
            return Left > Right;
        } );
    }, true );
}

LUMEN_TEST( Algo_Sort, NonTrivialValueType )
{
    Helpers::CheckStringSorter( [] ( TVector<FString> &Values )
    {
        Algo::Sort( Values );
    } );
}

LUMEN_TEST( Algo_Sort, SpanSortsOnlyItsWindow )
{
    TVector<Int32> Values;
    Helpers::FillShape( Values, 1000, Helpers::ESortShape::Random, 5ULL );
    const TVector<Int32> Original = Values;

    Algo::Sort( TSpan<Int32>( Values.data() + 100, 500 ) );

    LUMEN_CHECK_TRUE( std::is_sorted( Values.begin() + 100, Values.begin() + 600 ) );
    LUMEN_CHECK_TRUE( std::equal( Values.begin(), Values.begin() + 100, Original.begin() ) );
    LUMEN_CHECK_TRUE( std::equal( Values.begin() + 600, Values.end(), Original.begin() + 600 ) );
}

LUMEN_TEST( Algo_Sort, FixedArray )
{
    TArray<Int32, 8> Values{ { 5, 3, 8, 1, 9, 2, 7, 4 } };

    Algo::Sort( Values );

    LUMEN_CHECK_TRUE( std::ranges::is_sorted( Values ) );
}

} // namespace Lumen
