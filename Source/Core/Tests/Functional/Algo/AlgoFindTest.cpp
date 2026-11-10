/**
 * @file AlgoFindTest.cpp
 * @brief Exhaustive functional tests for every Algo::Find* overload.
 */

#include <Algo/Find.hpp>
#include <Tests/UnitTest.hpp>

#include <Memory/Span.hpp>
#include <Templates/Array.hpp>
#include <Templates/Vector.hpp>

#include <type_traits>
#include <utility>

namespace Lumen
{

namespace
{

    /** @brief Named predicates so tests read as specs and can be used in static_asserts. */
    [[nodiscard]] Bool IsPositive ( const Int32 V ) noexcept
    {
        return V > 0;
    }
    [[nodiscard]] Bool IsEven ( const Int32 V ) noexcept
    {
        return ( V % 2 ) == 0;
    }
    [[nodiscard]] Bool AlwaysFalse ( const Int32 ) noexcept
    {
        return false;
    }
    [[nodiscard]] Bool AlwaysTrue ( const Int32 ) noexcept
    {
        return true;
    }
    [[nodiscard]] Bool LessThan ( const Int32 L, const Int32 R ) noexcept
    {
        return L < R;
    }
    [[nodiscard]] Bool SameMod3 ( const Int32 L, const Int32 R ) noexcept
    {
        return ( L % 3 ) == ( R % 3 );
    }

    [[nodiscard]] TVector<Int32> MakeRange ( const std::initializer_list<Int32> Values )
    {
        return TVector<Int32>( Values );
    }

    /** @brief Canonical six-element sequence used by most tests. */
    [[nodiscard]] TVector<Int32> Canonical ()
    {
        return MakeRange( { 4, 8, 15, 16, 23, 42 } );
    }

} // namespace

/**
 * Compile-time: every overload yields the exact expected type.
 * A refactor that changes an overload's return type fails here before runtime.
 */

// Range overloads (return raw pointer).
static_assert( std::is_same_v<decltype( Algo::FindIf( std::declval<TVector<Int32> &>(), &IsPositive ) ), Int32 *> );
static_assert( std::is_same_v<decltype( Algo::Find( std::declval<TVector<Int32> &>(), Int32{} ) ), Int32 *> );
static_assert( std::is_same_v<decltype( Algo::FindIfNot( std::declval<TVector<Int32> &>(), &IsEven ) ), Int32 *> );

// Range overloads on a const container (return pointer-to-const).
static_assert( std::is_same_v<decltype( Algo::FindIf( std::declval<const TVector<Int32> &>(), &IsPositive ) ), const Int32 *> );
static_assert( std::is_same_v<decltype( Algo::Find( std::declval<const TVector<Int32> &>(), Int32{} ) ), const Int32 *> );
static_assert( std::is_same_v<decltype( Algo::FindIfNot( std::declval<const TVector<Int32> &>(), &IsEven ) ), const Int32 *> );

// Range *OrNone overloads (return a COPY, not a pointer).
static_assert(
    std::is_same_v<decltype( Algo::FindIfOrNone( std::declval<TVector<Int32> &>(), &IsPositive ) ), TOptional<Int32>> );
static_assert( std::is_same_v<decltype( Algo::FindOrNone( std::declval<TVector<Int32> &>(), Int32{} ) ), TOptional<Int32>> );
static_assert( std::is_same_v<decltype( Algo::FindIfNotOrNone( std::declval<TVector<Int32> &>(), &IsEven ) ), TOptional<Int32>> );

// Iterator overloads (return the iterator type).
using FIntIterator      = TVector<Int32>::iterator;
using FIntConstIterator = TVector<Int32>::const_iterator;

static_assert( std::is_same_v<decltype( Algo::FindIf( std::declval<FIntIterator>(), std::declval<FIntIterator>(), &IsPositive ) ),
                              FIntIterator> );
static_assert(
    std::is_same_v<decltype( Algo::Find( std::declval<FIntIterator>(), std::declval<FIntIterator>(), Int32{} ) ), FIntIterator> );
static_assert( std::is_same_v<decltype( Algo::FindIfNot( std::declval<FIntIterator>(), std::declval<FIntIterator>(), &IsEven ) ),
                              FIntIterator> );
static_assert( std::is_same_v<
               decltype( Algo::FindIfEqual( std::declval<FIntIterator>(), std::declval<FIntIterator>(), Int32{}, &SameMod3 ) ),
               FIntIterator> );

static_assert(
    std::is_same_v<decltype( Algo::FindIfOrNone( std::declval<FIntIterator>(), std::declval<FIntIterator>(), &IsPositive ) ),
                   TOptional<FIntIterator>> );
static_assert( std::is_same_v<decltype( Algo::FindOrNone( std::declval<FIntIterator>(), std::declval<FIntIterator>(), Int32{} ) ),
                              TOptional<FIntIterator>> );
static_assert(
    std::is_same_v<decltype( Algo::FindIfNotOrNone( std::declval<FIntIterator>(), std::declval<FIntIterator>(), &IsEven ) ),
                   TOptional<FIntIterator>> );
static_assert( std::is_same_v<decltype( Algo::FindIfEqualOrNone(
                                  std::declval<FIntIterator>(), std::declval<FIntIterator>(), Int32{}, &SameMod3 ) ),
                              TOptional<FIntIterator>> );

/**
 * Iterator-based FindIf
 */

LUMEN_TEST( Algo_FindIf_Iter, FoundAtFirstElement )
{
    TVector<Int32> Values = Canonical();
    const auto It         = Algo::FindIf( Values.begin(), Values.end(), [] ( const Int32 V ) noexcept
            {
        return V == 4;
    } );
    LUMEN_CHECK_TRUE( It == Values.begin() );
    LUMEN_CHECK_EQ( *It, 4 );
}

LUMEN_TEST( Algo_FindIf_Iter, FoundAtMiddle )
{
    TVector<Int32> Values = Canonical();
    const auto It         = Algo::FindIf( Values.begin(), Values.end(), [] ( const Int32 V ) noexcept
            {
        return V == 15;
    } );
    LUMEN_CHECK_TRUE( It != Values.end() );
    LUMEN_CHECK_EQ( *It, 15 );
    LUMEN_CHECK_EQ( static_cast<USize>( It - Values.begin() ), 2U );
}

LUMEN_TEST( Algo_FindIf_Iter, FoundAtLast )
{
    TVector<Int32> Values = Canonical();
    const auto It         = Algo::FindIf( Values.begin(), Values.end(), [] ( const Int32 V ) noexcept
            {
        return V == 42;
    } );
    LUMEN_CHECK_TRUE( It == Values.end() - 1 );
    LUMEN_CHECK_EQ( *It, 42 );
}

LUMEN_TEST( Algo_FindIf_Iter, NotFound )
{
    TVector<Int32> Values = Canonical();
    const auto It         = Algo::FindIf( Values.begin(), Values.end(), [] ( const Int32 V ) noexcept
            {
        return V > 1000;
    } );
    LUMEN_CHECK_TRUE( It == Values.end() );
}

LUMEN_TEST( Algo_FindIf_Iter, EmptyRange )
{
    TVector<Int32> Values;
    const auto It = Algo::FindIf( Values.begin(), Values.end(), &AlwaysTrue );
    LUMEN_CHECK_TRUE( It == Values.end() );
}

LUMEN_TEST( Algo_FindIf_Iter, FirstMatchWinsOnDuplicates )
{
    TVector<Int32> Values = MakeRange( { 7, 7, 7 } );
    const auto It         = Algo::FindIf( Values.begin(), Values.end(), [] ( const Int32 V ) noexcept
            {
        return V == 7;
    } );
    LUMEN_CHECK_TRUE( It == Values.begin() );
}

LUMEN_TEST( Algo_FindIf_Iter, StopsAfterFirstMatch_PredicateCounted )
{
    TVector<Int32> Values = Canonical();
    Int32 Evaluations     = 0;
    const auto It         = Algo::FindIf( Values.begin(), Values.end(), [&Evaluations] ( const Int32 V ) noexcept
            {
        ++Evaluations;
        return V == 15;
    } );
    LUMEN_CHECK_TRUE( It != Values.end() );
    LUMEN_CHECK_EQ( Evaluations, 3 );
}

LUMEN_TEST( Algo_FindIf_Iter, WorksOnRawPointers )
{
    Int32 Raw[] = { 1, 2, 3, 4, 5 };
    Int32 *End  = Raw + 5;
    Int32 *It   = Algo::FindIf( Raw, End, [] ( const Int32 V ) noexcept
      {
        return V == 4;
    } );
    LUMEN_CHECK_TRUE( It == Raw + 3 );
    LUMEN_CHECK_EQ( *It, 4 );
}

LUMEN_TEST( Algo_FindIf_Iter, PredicateReturningInt )
{

    TVector<Int32> Values = Canonical();
    const auto It         = Algo::FindIf( Values.begin(), Values.end(), [] ( const Int32 V ) noexcept
            {
        return V == 16 ? 1 : 0;
    } );
    LUMEN_CHECK_TRUE( It != Values.end() );
    LUMEN_CHECK_EQ( *It, 16 );
}

LUMEN_TEST( Algo_FindIf_Iter, ConstRange )
{
    const TVector<Int32> Values = Canonical();
    const auto It               = Algo::FindIf( Values.begin(), Values.end(), [] ( const Int32 V ) noexcept
                  {
        return V == 23;
    } );
    LUMEN_CHECK_TRUE( It != Values.end() );
    LUMEN_CHECK_EQ( *It, 23 );
}

/**
 * Iterator-based Find
 */

LUMEN_TEST( Algo_Find_Iter, Found )
{
    TVector<Int32> Values = Canonical();
    const auto It         = Algo::Find( Values.begin(), Values.end(), 23 );
    LUMEN_CHECK_TRUE( It != Values.end() );
    LUMEN_CHECK_EQ( *It, 23 );
}

LUMEN_TEST( Algo_Find_Iter, NotFound )
{
    TVector<Int32> Values = Canonical();
    const auto It         = Algo::Find( Values.begin(), Values.end(), 999 );
    LUMEN_CHECK_TRUE( It == Values.end() );
}

LUMEN_TEST( Algo_Find_Iter, EmptyRange )
{
    TVector<Int32> Values;
    const auto It = Algo::Find( Values.begin(), Values.end(), 1 );
    LUMEN_CHECK_TRUE( It == Values.end() );
}

LUMEN_TEST( Algo_Find_Iter, ReturnsFirstOfDuplicates )
{
    TVector<Int32> Values = MakeRange( { 3, 5, 3, 5 } );
    const auto It         = Algo::Find( Values.begin(), Values.end(), 5 );
    LUMEN_CHECK_TRUE( It == Values.begin() + 1 );
}

/**
 * Iterator-based FindIfNot
 */

LUMEN_TEST( Algo_FindIfNot_Iter, FoundFirstNonEven )
{
    TVector<Int32> Values = MakeRange( { 2, 4, 6, 7, 8 } );
    const auto It         = Algo::FindIfNot( Values.begin(), Values.end(), &IsEven );
    LUMEN_CHECK_TRUE( It == Values.begin() + 3 );
    LUMEN_CHECK_EQ( *It, 7 );
}

LUMEN_TEST( Algo_FindIfNot_Iter, AllMatch_ReturnsEnd )
{
    TVector<Int32> Values = MakeRange( { 2, 4, 6 } );
    const auto It         = Algo::FindIfNot( Values.begin(), Values.end(), &IsEven );
    LUMEN_CHECK_TRUE( It == Values.end() );
}

LUMEN_TEST( Algo_FindIfNot_Iter, EmptyRange )
{
    TVector<Int32> Values;
    const auto It = Algo::FindIfNot( Values.begin(), Values.end(), &IsEven );
    LUMEN_CHECK_TRUE( It == Values.end() );
}

/**
 * Iterator-based FindIfEqual
 */

LUMEN_TEST( Algo_FindIfEqual_Iter, FoundByComparator )
{
    TVector<Int32> Values = MakeRange( { 10, 20, 30, 40 } );

    const auto It = Algo::FindIfEqual( Values.begin(), Values.end(), 40, &SameMod3 );
    LUMEN_CHECK_TRUE( It == Values.begin() );
    LUMEN_CHECK_EQ( *It, 10 );
}

LUMEN_TEST( Algo_FindIfEqual_Iter, NotFound )
{
    TVector<Int32> Values = MakeRange( { 10, 20, 30 } );
    const auto It         = Algo::FindIfEqual( Values.begin(), Values.end(), 0, &LessThan );
    LUMEN_CHECK_TRUE( It == Values.end() );
}

LUMEN_TEST( Algo_FindIfEqual_Iter, EmptyRange )
{
    TVector<Int32> Values;
    const auto It = Algo::FindIfEqual( Values.begin(), Values.end(), 0, &SameMod3 );
    LUMEN_CHECK_TRUE( It == Values.end() );
}

/**
 * Iterator-based *OrNone variants
 */

LUMEN_TEST( Algo_FindIfOrNone_Iter, Found )
{
    TVector<Int32> Values = Canonical();
    const auto Result     = Algo::FindIfOrNone( Values.begin(), Values.end(), [] ( const Int32 V ) noexcept
        {
        return V == 16;
    } );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_TRUE( *Result == Values.begin() + 3 );
    LUMEN_CHECK_EQ( **Result, 16 );
}

LUMEN_TEST( Algo_FindIfOrNone_Iter, NotFound )
{
    TVector<Int32> Values = Canonical();
    const auto Result     = Algo::FindIfOrNone( Values.begin(), Values.end(), &AlwaysFalse );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Algo_FindIfOrNone_Iter, EmptyRange )
{
    TVector<Int32> Values;
    const auto Result = Algo::FindIfOrNone( Values.begin(), Values.end(), &AlwaysTrue );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Algo_FindOrNone_Iter, Found )
{
    TVector<Int32> Values = Canonical();
    const auto Result     = Algo::FindOrNone( Values.begin(), Values.end(), 42 );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( **Result, 42 );
}

LUMEN_TEST( Algo_FindOrNone_Iter, NotFound )
{
    TVector<Int32> Values = Canonical();
    const auto Result     = Algo::FindOrNone( Values.begin(), Values.end(), 999 );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Algo_FindIfNotOrNone_Iter, Found )
{
    TVector<Int32> Values = MakeRange( { 2, 4, 7 } );
    const auto Result     = Algo::FindIfNotOrNone( Values.begin(), Values.end(), &IsEven );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( **Result, 7 );
}

LUMEN_TEST( Algo_FindIfNotOrNone_Iter, NotFound )
{
    TVector<Int32> Values = MakeRange( { 2, 4, 6 } );
    const auto Result     = Algo::FindIfNotOrNone( Values.begin(), Values.end(), &IsEven );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Algo_FindIfEqualOrNone_Iter, Found )
{
    TVector<Int32> Values = MakeRange( { 10, 20, 30 } );
    const auto Result     = Algo::FindIfEqualOrNone( Values.begin(), Values.end(), 30, &SameMod3 );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( **Result, 30 );
}

LUMEN_TEST( Algo_FindIfEqualOrNone_Iter, NotFound )
{
    TVector<Int32> Values = MakeRange( { 10, 20, 30 } );
    const auto Result     = Algo::FindIfEqualOrNone( Values.begin(), Values.end(), 0, &LessThan );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

/**
 * Range-based FindIf  —  the overload most vulnerable to the ForEachUntil refactor.
 *  - must return the EXACT address of the matching element,
 *  - must allow mutation through that pointer.
 */

LUMEN_TEST( Algo_FindIf_Range, ReturnsExactPointerToMiddleElement )
{
    TVector<Int32> Values = Canonical();

    Int32 *Found = Algo::FindIf( Values, [] ( const Int32 V ) noexcept
    {
        return V == 15;
    } );

    LUMEN_CHECK_TRUE( Found != nullptr );
    LUMEN_CHECK_TRUE( Found == &Values[2] );
    LUMEN_CHECK_TRUE( Found == Values.data() + 2 );
    LUMEN_CHECK_EQ( *Found, 15 );
}

LUMEN_TEST( Algo_FindIf_Range, ReturnsPointerToFirstElement )
{
    TVector<Int32> Values = Canonical();
    Int32 *Found          = Algo::FindIf( Values, [] ( const Int32 V ) noexcept
             {
        return V == 4;
    } );
    LUMEN_CHECK_TRUE( Found == &Values[0] );
}

LUMEN_TEST( Algo_FindIf_Range, ReturnsPointerToLastElement )
{
    TVector<Int32> Values = Canonical();
    Int32 *Found          = Algo::FindIf( Values, [] ( const Int32 V ) noexcept
             {
        return V == 42;
    } );
    LUMEN_CHECK_TRUE( Found == &Values.back() );
}

LUMEN_TEST( Algo_FindIf_Range, NotFound_ReturnsNull )
{
    TVector<Int32> Values = Canonical();
    Int32 *Found          = Algo::FindIf( Values, &AlwaysFalse );
    LUMEN_CHECK_TRUE( Found == nullptr );
}

LUMEN_TEST( Algo_FindIf_Range, EmptyRange_ReturnsNull )
{
    TVector<Int32> Values;
    Int32 *Found = Algo::FindIf( Values, &AlwaysTrue );
    LUMEN_CHECK_TRUE( Found == nullptr );
}

LUMEN_TEST( Algo_FindIf_Range, RegressionForEachUntilDoesNotLoseTheElement )
{
    TVector<Int32> Values = Canonical();

    Int32 *Found = Algo::FindIf( Values, [] ( const Int32 V ) noexcept
    {
        return V == 16;
    } );

    LUMEN_CHECK_MSG( Found != nullptr, "FindIf returned nullptr on a present element — ForEachUntil refactor regression" );
    LUMEN_CHECK_TRUE( Found == &Values[3] );
    LUMEN_CHECK_EQ( *Found, 16 );
}

LUMEN_TEST( Algo_FindIf_Range, MutationThroughPointerHitsTheContainer )
{
    TVector<Int32> Values = Canonical();

    Int32 *Found = Algo::FindIf( Values, [] ( const Int32 V ) noexcept
    {
        return V == 23;
    } );
    LUMEN_CHECK_TRUE( Found != nullptr );
    if ( Found )
    {
        *Found = 999;
    }

    LUMEN_CHECK_EQ( Values[4], 999 );
    LUMEN_CHECK_TRUE( Values == MakeRange( { 4, 8, 15, 16, 999, 42 } ) );
}

LUMEN_TEST( Algo_FindIf_Range, ConstRangeReturnsConstPointer )
{
    const TVector<Int32> Values = Canonical();
    const Int32 *Found          = Algo::FindIf( Values, [] ( const Int32 V ) noexcept
             {
        return V == 16;
    } );
    LUMEN_CHECK_TRUE( Found == &Values[3] );
}

LUMEN_TEST( Algo_FindIf_Range, WorksOnFixedArray )
{
    TArray<Int32, 5> Values{ { 9, 8, 7, 6, 5 } };
    Int32 *Found = Algo::FindIf( Values, [] ( const Int32 V ) noexcept
    {
        return V == 7;
    } );
    LUMEN_CHECK_TRUE( Found == &Values[2] );
}

LUMEN_TEST( Algo_FindIf_Range, WorksOnSpan )
{
    TVector<Int32> Storage = Canonical();
    TSpan<Int32> View( Storage.data() + 1, 3 );
    Int32 *Found = Algo::FindIf( View, [] ( const Int32 V ) noexcept
    {
        return V == 15;
    } );
    LUMEN_CHECK_TRUE( Found == Storage.data() + 2 );
}

LUMEN_TEST( Algo_FindIf_Range, StopsAfterFirstMatch_PredicateCounted )
{
    TVector<Int32> Values = Canonical();
    Int32 Evaluations     = 0;
    Int32 *Found          = Algo::FindIf( Values, [&Evaluations] ( const Int32 V ) noexcept
             {
        ++Evaluations;
        return V == 15;
    } );
    LUMEN_CHECK_TRUE( Found == &Values[2] );
    LUMEN_CHECK_EQ( Evaluations, 3 );
}

LUMEN_TEST( Algo_FindIf_Range, NonTrivialValueType )
{
    TVector<FString> Values{ FString( "alpha" ), FString( "beta" ), FString( "gamma" ) };
    FString *Found = Algo::FindIf( Values, [] ( const FString &V )
    {
        return V == "beta";
    } );
    LUMEN_CHECK_TRUE( Found == &Values[1] );
}

/**
 * Range-based Find
 */

LUMEN_TEST( Algo_Find_Range, Found_ReturnsExactPointer )
{
    TVector<Int32> Values = Canonical();
    Int32 *Found          = Algo::Find( Values, 16 );
    LUMEN_CHECK_TRUE( Found == &Values[3] );
}

LUMEN_TEST( Algo_Find_Range, ReturnsFirstOfDuplicates )
{
    TVector<Int32> Values = MakeRange( { 3, 5, 3, 5 } );
    Int32 *Found          = Algo::Find( Values, 5 );
    LUMEN_CHECK_TRUE( Found == &Values[1] );
}

LUMEN_TEST( Algo_Find_Range, NotFound_ReturnsNull )
{
    TVector<Int32> Values = Canonical();
    Int32 *Found          = Algo::Find( Values, 999 );
    LUMEN_CHECK_TRUE( Found == nullptr );
}

LUMEN_TEST( Algo_Find_Range, EmptyRange_ReturnsNull )
{
    TVector<Int32> Values;
    Int32 *Found = Algo::Find( Values, 1 );
    LUMEN_CHECK_TRUE( Found == nullptr );
}

LUMEN_TEST( Algo_Find_Range, MutationThroughPointer )
{
    TVector<Int32> Values = Canonical();
    Int32 *Found          = Algo::Find( Values, 42 );
    LUMEN_CHECK_TRUE( Found != nullptr );
    if ( Found )
    {
        *Found = 100;
    }
    LUMEN_CHECK_EQ( Values.back(), 100 );
}

LUMEN_TEST( Algo_Find_Range, ConstRangeReturnsConstPointer )
{
    const TVector<Int32> Values = Canonical();
    const Int32 *Found          = Algo::Find( Values, 8 );
    LUMEN_CHECK_TRUE( Found == &Values[1] );
}

LUMEN_TEST( Algo_Find_Range, WorksOnFixedArray )
{
    TArray<Int32, 4> Values{ { 1, 2, 3, 4 } };
    Int32 *Found = Algo::Find( Values, 3 );
    LUMEN_CHECK_TRUE( Found == &Values[2] );
}

LUMEN_TEST( Algo_Find_Range, WorksOnSpan )
{
    TVector<Int32> Storage = MakeRange( { 10, 20, 30, 40, 50 } );
    TSpan<Int32> View( Storage.data() + 1, 3 );
    Int32 *Found = Algo::Find( View, 40 );
    LUMEN_CHECK_TRUE( Found == Storage.data() + 3 );
}

LUMEN_TEST( Algo_Find_Range, NonTrivialValueType )
{
    TVector<FString> Values{ FString( "one" ), FString( "two" ), FString( "three" ) };
    FString *Found = Algo::Find( Values, FString( "two" ) );
    LUMEN_CHECK_TRUE( Found == &Values[1] );
}

/**
 * Range-based FindIfNot
 */

LUMEN_TEST( Algo_FindIfNot_Range, Found_ReturnsExactPointer )
{
    TVector<Int32> Values = MakeRange( { 2, 4, 7, 8 } );
    Int32 *Found          = Algo::FindIfNot( Values, &IsEven );
    LUMEN_CHECK_TRUE( Found == &Values[2] );
    LUMEN_CHECK_EQ( *Found, 7 );
}

LUMEN_TEST( Algo_FindIfNot_Range, AllMatch_ReturnsNull )
{
    TVector<Int32> Values = MakeRange( { 2, 4, 6 } );
    Int32 *Found          = Algo::FindIfNot( Values, &IsEven );
    LUMEN_CHECK_TRUE( Found == nullptr );
}

LUMEN_TEST( Algo_FindIfNot_Range, EmptyRange_ReturnsNull )
{
    TVector<Int32> Values;
    Int32 *Found = Algo::FindIfNot( Values, &IsEven );
    LUMEN_CHECK_TRUE( Found == nullptr );
}

LUMEN_TEST( Algo_FindIfNot_Range, MutationThroughPointer )
{
    TVector<Int32> Values = MakeRange( { 2, 4, 7 } );
    Int32 *Found          = Algo::FindIfNot( Values, &IsEven );
    LUMEN_CHECK_TRUE( Found != nullptr );
    if ( Found )
    {
        *Found = 0;
    }
    LUMEN_CHECK_EQ( Values[2], 0 );
}

/**
 * Range-based *OrNone variants  —  return TOptional<Value> (a COPY, not a pointer).
 */

LUMEN_TEST( Algo_FindIfOrNone_Range, Found_ContainsValue )
{
    TVector<Int32> Values = Canonical();
    const auto Result     = Algo::FindIfOrNone( Values, [] ( const Int32 V ) noexcept
        {
        return V == 23;
    } );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 23 );
}

LUMEN_TEST( Algo_FindIfOrNone_Range, NotFound_Empty )
{
    TVector<Int32> Values = Canonical();
    const auto Result     = Algo::FindIfOrNone( Values, &AlwaysFalse );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Algo_FindIfOrNone_Range, EmptyRange )
{
    TVector<Int32> Values;
    const auto Result = Algo::FindIfOrNone( Values, &AlwaysTrue );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Algo_FindOrNone_Range, Found )
{
    TVector<Int32> Values = Canonical();
    const auto Result     = Algo::FindOrNone( Values, 16 );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 16 );
}

LUMEN_TEST( Algo_FindOrNone_Range, NotFound )
{
    TVector<Int32> Values = Canonical();
    const auto Result     = Algo::FindOrNone( Values, 999 );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Algo_FindIfNotOrNone_Range, Found )
{
    TVector<Int32> Values = MakeRange( { 2, 4, 9 } );
    const auto Result     = Algo::FindIfNotOrNone( Values, &IsEven );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 9 );
}

LUMEN_TEST( Algo_FindIfNotOrNone_Range, NotFound )
{
    TVector<Int32> Values = MakeRange( { 2, 4, 6 } );
    const auto Result     = Algo::FindIfNotOrNone( Values, &IsEven );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Algo_FindIfOrNone_Range, NonTrivialValueType_CopiesValue )
{
    TVector<FString> Values{ FString( "a" ), FString( "bb" ), FString( "ccc" ) };
    const auto Result = Algo::FindIfOrNone( Values, [] ( const FString &V )
    {
        return V.size() == 2;
    } );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, "bb" );
}

/**
 * Cross-overload identity and interop: a refactor that changes one overload in
 * isolation still has to keep it consistent with the other.
 */

LUMEN_TEST( Algo_Find, RangeAndIteratorOverloadsAgree )
{
    TVector<Int32> Values = Canonical();

    Int32 *RangeResult    = Algo::FindIf( Values, [] ( const Int32 V ) noexcept
       {
        return V == 16;
    } );
    const auto IterResult = Algo::FindIf( Values.begin(), Values.end(), [] ( const Int32 V ) noexcept
    {
        return V == 16;
    } );

    LUMEN_CHECK_TRUE( RangeResult != nullptr );
    LUMEN_CHECK_TRUE( IterResult != Values.end() );
    LUMEN_CHECK_TRUE( RangeResult == &*IterResult );
    LUMEN_CHECK_TRUE( IterResult == Values.begin() + 3 );
}

LUMEN_TEST( Algo_Find, PointerIteratorOverloadOnReturnedPointer )
{
    TVector<Int32> Values = Canonical();

    Int32 *Found = Algo::Find( Values, 23 );
    LUMEN_CHECK_TRUE( Found != nullptr );

    Int32 *Next = Algo::FindIf( Found, Values.data() + Values.size(), [] ( const Int32 V ) noexcept
    {
        return V == 42;
    } );
    LUMEN_CHECK_TRUE( Next == &Values.back() );
}

} // namespace Lumen
