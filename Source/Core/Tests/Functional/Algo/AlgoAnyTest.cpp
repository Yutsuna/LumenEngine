/**
 * @file AlgoAnyTest.cpp
 * @brief Functional tests for Algo::AnyOf.
 */

#include "Tests/UnitTestsAssert.hpp"
#include <Algo/Any.hpp>
#include <Tests/UnitTest.hpp>

#include <Memory/Span.hpp>
#include <String/String.hpp>
#include <Templates/Array.hpp>
#include <Templates/Vector.hpp>

#include <initializer_list>
#include <type_traits>
#include <utility>

namespace Lumen
{

namespace
{

    /**
     * Hepers
     */

    [[nodiscard]] constexpr Bool IsPositive ( const Int32 V ) noexcept
    {
        return V > 0;
    }

    [[nodiscard]] constexpr Bool AlwaysFalse ( const Int32 LUMEN_UNUSED V ) noexcept
    {
        return false;
    }

    [[nodiscard]] constexpr Bool AlwaysTrue ( const Int32 LUMEN_UNUSED V ) noexcept
    {
        return true;
    }

    [[nodiscard]] TVector<Int32> MakeRange ( const std::initializer_list<Int32> Values )
    {
        return Values;
    }

    /** @brief A predicate result that is only implicitly convertible to Bool. */
    struct FTruthyResult
    {
        Int32 Value = 0;

        [[nodiscard]] constexpr operator Bool () const noexcept
        {
            return Value != 0;
        }
    };

    /** @brief Element exposing a const member function usable as a predicate. */
    struct FItem
    {
        Int32 Id = 0;

        [[nodiscard]] constexpr Bool IsReady () const noexcept
        {
            return Id == 7;
        }
    };

    /**
     * Tests
     */

    /**
     * Compile-time contract: the algorithm always yields Bool and is constexpr.
     */

    static_assert( std::is_same_v<decltype( Algo::AnyOf( std::declval<TVector<Int32> &>(), &IsPositive ) ), Bool> );
    static_assert( std::is_same_v<decltype( Algo::AnyOf( std::declval<const TVector<Int32> &>(), &IsPositive ) ), Bool> );
    static_assert( std::is_same_v<decltype( Algo::AnyOf( std::declval<TArray<Int32, 4> &>(), &IsPositive ) ), Bool> );

    /**
     * Basic range behaviour
     */

    LUMEN_TEST( Algo_AnyOf, ConstexprAnyOf )
    {
        constexpr Int32 ConstexprValues[] = { 1, 3, 5 };

        constexpr auto ResultA = Algo::AnyOf( ConstexprValues, [] ( const Int32 V ) noexcept -> Bool
        {
            return V == 3;
        } );

        constexpr auto ResultB = Algo::AnyOf( ConstexprValues, [] ( const Int32 V ) noexcept -> Bool
        {
            return V == 4;
        } );

        constexpr auto ResultC = Algo::AnyOf( ConstexprValues, &AlwaysFalse );

        LUMEN_CHECK_TRUE( ResultA );
        LUMEN_CHECK_TRUE( not ResultB );
        LUMEN_CHECK_TRUE( not ResultC );
    }

    LUMEN_TEST( Algo_AnyOf, FoundAtFirstElement )
    {
        TVector<Int32> Values = MakeRange( { 7, 8, 9 } );
        const Bool Result     = Algo::AnyOf( Values, [] ( const Int32 V ) noexcept
            {
            return V == 7;
        } );

        LUMEN_CHECK_TRUE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, FoundAtMiddle )
    {
        TVector<Int32> Values = MakeRange( { 4, 8, 15, 16, 23, 42 } );
        const Bool Result     = Algo::AnyOf( Values, [] ( const Int32 V ) noexcept
            {
            return V == 15;
        } );

        LUMEN_CHECK_TRUE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, FoundAtLast )
    {
        TVector<Int32> Values = MakeRange( { 4, 8, 15 } );
        const Bool Result     = Algo::AnyOf( Values, [] ( const Int32 V ) noexcept
            {
            return V == 15;
        } );

        LUMEN_CHECK_TRUE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, NotFound )
    {
        TVector<Int32> Values = MakeRange( { 4, 8, 15 } );
        const Bool Result     = Algo::AnyOf( Values, AlwaysFalse );

        LUMEN_CHECK_FALSE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, EmptyRange )
    {
        TVector<Int32> Values;
        const Bool Result = Algo::AnyOf( Values, &AlwaysTrue );

        LUMEN_CHECK_FALSE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, SingleElementBothOutcomes )
    {
        TVector<Int32> Values{ 3 };

        LUMEN_CHECK_TRUE( Algo::AnyOf( Values, &IsPositive ) );
        LUMEN_CHECK_FALSE( Algo::AnyOf( Values, AlwaysFalse ) );
    }

    LUMEN_TEST( Algo_AnyOf, DuplicatesStillReturnTrue )
    {
        TVector<Int32> Values = MakeRange( { 7, 7, 7 } );
        const Bool Result     = Algo::AnyOf( Values, [] ( const Int32 V ) noexcept
            {
            return V == 7;
        } );

        LUMEN_CHECK_TRUE( Result );
    }

    /**
     * Short-circuiting
     */

    LUMEN_TEST( Algo_AnyOf, StopsAfterFirstMatch )
    {
        TVector<Int32> Values = MakeRange( { 1, 2, 3, 4, 5 } );
        Int32 Evaluations     = 0;

        const Bool Result = Algo::AnyOf( Values, [&Evaluations] ( const Int32 V ) noexcept
        {
            ++Evaluations;
            return V == 3;
        } );

        LUMEN_CHECK_TRUE( Result );
        LUMEN_CHECK_EQ( Evaluations, 3 );
    }

    LUMEN_TEST( Algo_AnyOf, VisitsEveryElementWhenNoMatch )
    {
        TVector<Int32> Values = MakeRange( { 1, 2, 3, 4, 5 } );
        Int32 Evaluations     = 0;

        const Bool Result = Algo::AnyOf( Values, [&Evaluations] ( const Int32 ) noexcept
        {
            ++Evaluations;
            return false;
        } );

        LUMEN_CHECK_FALSE( Result );
        LUMEN_CHECK_EQ( Evaluations, 5 );
    }

    LUMEN_TEST( Algo_AnyOf, EmptyRangeNeverCallsPredicate )
    {
        TVector<Int32> Values;
        Int32 Evaluations = 0;

        const Bool Result = Algo::AnyOf( Values, [&Evaluations] ( const Int32 ) noexcept
        {
            ++Evaluations;
            return true;
        } );

        LUMEN_CHECK_FALSE( Result );
        LUMEN_CHECK_EQ( Evaluations, 0 );
    }

    /**
     * Range kinds
     */

    LUMEN_TEST( Algo_AnyOf, ConstRange )
    {
        const TVector<Int32> Values = MakeRange( { -1, 4, -2 } );
        const Bool Result           = Algo::AnyOf( Values, &IsPositive );

        LUMEN_CHECK_TRUE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, WorksOnFixedArray )
    {
        TArray<Int32, 4> Values{ { 1, 2, 3, 4 } };
        const Bool Result = Algo::AnyOf( Values, [] ( const Int32 V ) noexcept
        {
            return V == 3;
        } );

        LUMEN_CHECK_TRUE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, WorksOnSpanAndRespectsItsBounds )
    {
        TVector<Int32> Storage = MakeRange( { 11, -2, -4, 33 } );
        TSpan<Int32> View( Storage.data() + 1, 2 );

        LUMEN_CHECK_FALSE( Algo::AnyOf( View, &IsPositive ) );

        TSpan<Int32> FullView( Storage.data(), 4 );
        LUMEN_CHECK_TRUE( Algo::AnyOf( FullView, &IsPositive ) );
    }

    LUMEN_TEST( Algo_AnyOf, WorksOnRawArray )
    {
        Int32 Values[]    = { 1, 2, 3 };
        const Bool Result = Algo::AnyOf( Values, [] ( const Int32 V ) noexcept
        {
            return V == 2;
        } );

        LUMEN_CHECK_TRUE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, WorksOnTemporaryRange )
    {
        const Bool Result = Algo::AnyOf( MakeRange( { 1, 2, 3 } ), [] ( const Int32 V ) noexcept
        {
            return V == 2;
        } );

        LUMEN_CHECK_TRUE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, NonTrivialValueType )
    {
        TVector<FString> Values{ FString( "alpha" ), FString( "beta" ), FString( "gamma" ) };
        const Bool Result = Algo::AnyOf( Values, [] ( const FString &V )
        {
            return V == "beta";
        } );

        LUMEN_CHECK_TRUE( Result );
    }

    /**
     * Predicate forms
     */

    LUMEN_TEST( Algo_AnyOf, PredicateReturningInt )
    {
        TVector<Int32> Values = MakeRange( { 2, 4, 7 } );
        const Bool Result     = Algo::AnyOf( Values, [] ( const Int32 V ) noexcept
            {
            return V % 2;
        } );

        LUMEN_CHECK_TRUE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, PredicateReturningImplicitlyConvertibleType )
    {
        TVector<Int32> Values = MakeRange( { 2, 4, 6 } );

        LUMEN_CHECK_FALSE( Algo::AnyOf( Values, [] ( const Int32 ) noexcept
        {
            return FTruthyResult{ 0 };
        } ) );
        LUMEN_CHECK_TRUE( Algo::AnyOf( Values, [] ( const Int32 V ) noexcept
        {
            return FTruthyResult{ V };
        } ) );
    }

    LUMEN_TEST( Algo_AnyOf, PointerToMemberFunctionPredicate )
    {
        TVector<FItem> Items{ FItem{ 1 }, FItem{ 7 }, FItem{ 3 } };
        const Bool Result = Algo::AnyOf( Items, &FItem::IsReady );

        LUMEN_CHECK_TRUE( Result );
    }

    LUMEN_TEST( Algo_AnyOf, StatefulLvaluePredicate )
    {
        TVector<Int32> Values = MakeRange( { 1, 3, 6, 8 } );
        Int32 Calls           = 0;

        const auto Predicate = [&Calls] ( const Int32 V ) noexcept
        {
            ++Calls;
            return ( V % 2 ) == 0;
        };

        const Bool Result = Algo::AnyOf( Values, Predicate );

        LUMEN_CHECK_TRUE( Result );
        LUMEN_CHECK_EQ( Calls, 3 );
    }

    LUMEN_TEST( Algo_AnyOf, PredicateCanMutateElements )
    {
        TVector<Int32> Values = MakeRange( { 1, 2, 3 } );
        const Bool Result     = Algo::AnyOf( Values, [] ( Int32 &V ) noexcept
            {
            V += 10;
            return V == 12;
        } );

        LUMEN_CHECK_TRUE( Result );
        LUMEN_CHECK_TRUE( Values == MakeRange( { 11, 12, 3 } ) );
    }

} // namespace

} // namespace Lumen
