/**
 * @file AlgoForEachTest.cpp
 * @brief Functional tests for Algo::ForEach and Algo::ForEachUntil.
 */

#include <Algo/ForEach.hpp>
#include <Tests/UnitTest.hpp>

#include <Templates/Array.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace
{

    [[nodiscard]] TVector<Int32> MakeRange ( const std::initializer_list<Int32> Values )
    {
        return TVector<Int32>( Values );
    }

} // namespace

/**
 * ForEach (unconditional)
 */

LUMEN_TEST( Algo_ForEach, VisitsEveryElementInOrder )
{
    TVector<Int32> Values = MakeRange( { 1, 2, 3, 4, 5 } );
    TVector<Int32> Visited;

    Algo::ForEach( Values, [&Visited] ( const Int32 Value )
    {
        Visited.push_back( Value );
    } );

    LUMEN_CHECK_TRUE( Visited == Values );
}

LUMEN_TEST( Algo_ForEach, EmptyRangeNeverCallsFunc )
{
    TVector<Int32> Values;
    Int32 Calls = 0;

    Algo::ForEach( Values, [&Calls] ( const Int32 )
    {
        ++Calls;
    } );

    LUMEN_CHECK_EQ( Calls, 0 );
}

LUMEN_TEST( Algo_ForEach, MutatesElements )
{
    TVector<Int32> Values = MakeRange( { 1, 2, 3 } );

    Algo::ForEach( Values, [] ( Int32 &Value )
    {
        Value *= 10;
    } );

    LUMEN_CHECK_TRUE( Values == MakeRange( { 10, 20, 30 } ) );
}

LUMEN_TEST( Algo_ForEach, WorksOnFixedArray )
{
    TArray<Int32, 4> Values{ { 1, 2, 3, 4 } };
    Int32 Sum = 0;

    Algo::ForEach( Values, [&Sum] ( const Int32 Value )
    {
        Sum += Value;
    } );

    LUMEN_CHECK_EQ( Sum, 10 );
}

/**
 * ForEach (predicate)
 * @note This overload is intended to apply Func only to elements satisfying Predicate.
 */

LUMEN_TEST( Algo_ForEach, PredicateFiltersElements )
{
    TVector<Int32> Values = MakeRange( { 1, 2, 3, 4, 5, 6 } );
    TVector<Int32> Visited;

    Algo::ForEach( Values,
                   [] ( const Int32 Value ) noexcept
    {
        return ( Value % 2 ) == 0;
    }, [&Visited] ( const Int32 Value )
    {
        Visited.push_back( Value );
    } );

    LUMEN_CHECK_TRUE( Visited == MakeRange( { 2, 4, 6 } ) );
}

LUMEN_TEST( Algo_ForEach, PredicateNeverMatches )
{
    TVector<Int32> Values = MakeRange( { 1, 3, 5 } );
    Int32 Calls           = 0;

    Algo::ForEach( Values,
                   [] ( const Int32 ) noexcept
    {
        return false;
    }, [&Calls] ( const Int32 )
    {
        ++Calls;
    } );

    LUMEN_CHECK_EQ( Calls, 0 );
}

LUMEN_TEST( Algo_ForEach, PredicateAlwaysMatches )
{
    TVector<Int32> Values = MakeRange( { 1, 2, 3 } );
    Int32 Calls           = 0;

    Algo::ForEach( Values,
                   [] ( const Int32 ) noexcept
    {
        return true;
    }, [&Calls] ( const Int32 )
    {
        ++Calls;
    } );

    LUMEN_CHECK_EQ( Calls, 3 );
}

/**
 * ForEachUntil (unconditional)
 */

LUMEN_TEST( Algo_ForEachUntil, StopsWhenFuncReturnsTrue )
{
    TVector<Int32> Values = MakeRange( { 1, 2, 3, 4, 5 } );
    TVector<Int32> Visited;

    const Bool bStopped = Algo::ForEachUntil( Values, [&Visited] ( const Int32 Value )
    {
        Visited.push_back( Value );
        return Value == 3;
    } );

    LUMEN_CHECK_TRUE( bStopped );
    LUMEN_CHECK_TRUE( Visited == MakeRange( { 1, 2, 3 } ) );
}

LUMEN_TEST( Algo_ForEachUntil, ReturnsFalseWhenExhausted )
{
    TVector<Int32> Values = MakeRange( { 1, 2, 3 } );
    Int32 Calls           = 0;

    const Bool bStopped = Algo::ForEachUntil( Values, [&Calls] ( const Int32 )
    {
        ++Calls;
        return false;
    } );

    LUMEN_CHECK_FALSE( bStopped );
    LUMEN_CHECK_EQ( Calls, 3 );
}

LUMEN_TEST( Algo_ForEachUntil, StopsOnFirstElement )
{
    TVector<Int32> Values = MakeRange( { 5, 6, 7 } );
    Int32 Calls           = 0;

    const Bool bStopped = Algo::ForEachUntil( Values, [&Calls] ( const Int32 )
    {
        ++Calls;
        return true;
    } );

    LUMEN_CHECK_TRUE( bStopped );
    LUMEN_CHECK_EQ( Calls, 1 );
}

/**
 * ForEachUntil (predicate + func)
 */

LUMEN_TEST( Algo_ForEachUntil, PredicateGatesTermination )
{
    TVector<Int32> Values = MakeRange( { 1, 2, 3, 4, 5 } );
    TVector<Int32> Visited;

    /**
     * INFO: Only elements that satisfy the predicate invoke Func, and only such
     *       elements are allowed to trigger the early stop.
     */
    const Bool bStopped = Algo::ForEachUntil( Values,
                                              [] ( const Int32 Value ) noexcept
    {
        return ( Value % 2 ) == 0;
    }, [&Visited] ( const Int32 Value )
    {
        Visited.push_back( Value );
        return Value == 4;
    } );

    LUMEN_CHECK_TRUE( bStopped );
    LUMEN_CHECK_TRUE( Visited == MakeRange( { 2, 4 } ) );
}

LUMEN_TEST( Algo_ForEachUntil, PredicateRejectsAllReturnsFalse )
{
    TVector<Int32> Values = MakeRange( { 1, 3, 5 } );
    Int32 Calls           = 0;

    const Bool bStopped = Algo::ForEachUntil( Values,
                                              [] ( const Int32 ) noexcept
    {
        return false;
    }, [&Calls] ( const Int32 )
    {
        ++Calls;
        return true;
    } );

    LUMEN_CHECK_FALSE( bStopped );
    LUMEN_CHECK_EQ( Calls, 0 );
}

LUMEN_TEST( Algo_ForEachUntil, CallsFuncOnlyForMatchingElements )
{
    TVector<Int32> Values = MakeRange( { 1, 2, 3, 4, 5, 6 } );
    TVector<Int32> Visited;

    Algo::ForEachUntil( Values,
                        [] ( const Int32 Value ) noexcept
    {
        return ( Value % 2 ) == 0;
    }, [&Visited] ( const Int32 Value )
    {
        Visited.push_back( Value );
        return false;
    } );

    LUMEN_CHECK_TRUE( Visited == MakeRange( { 2, 4, 6 } ) );
}

/**
 * Realistic use: short-circuit search-like usage
 */

LUMEN_TEST( Algo_ForEachUntil, EarlyStopStopsIteration )
{
    TVector<Int32> Values = MakeRange( { 10, 20, 30, 40, 50 } );
    Int32 Sum             = 0;

    Algo::ForEachUntil( Values, [&Sum] ( const Int32 Value )
    {
        Sum += Value;
        return Sum >= 30;
    } );

    LUMEN_CHECK_EQ( Sum, 30 );
}

} // namespace Lumen
