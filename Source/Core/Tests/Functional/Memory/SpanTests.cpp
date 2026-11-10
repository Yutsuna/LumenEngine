/**
 * @file SpanTests.cpp
 * @brief Functional unit tests for TSpan (non-owning view over contiguous sequences).
 */

#include <Memory/Span.hpp>
#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

#include <type_traits>

namespace Lumen
{

namespace
{

    static_assert( std::is_same_v<TSpan<Int32>, std::span<Int32>> );
    static_assert( std::is_same_v<TSpan<Int32, 4>, std::span<Int32, 4>> );
    static_assert( TSpan<Int32>::extent == std::dynamic_extent );

} // namespace

/**
 * Span — Construction
 */

LUMEN_TEST( Span_Constructor, FromArrayDeducesTheLength )
{
    Int32 Values[4] = { 1, 2, 3, 4 };

    TSpan<Int32> View{ Values };

    LUMEN_CHECK_EQ( View.size(), 4U );
    LUMEN_CHECK_TRUE( View.data() == Values );
    LUMEN_CHECK_EQ( View[0], 1 );
    LUMEN_CHECK_EQ( View[3], 4 );
}

LUMEN_TEST( Span_Constructor, FromPointerAndLength )
{
    Int32 Values[4] = { 1, 2, 3, 4 };

    TSpan<Int32> View{ Values, 3 };

    LUMEN_CHECK_EQ( View.size(), 3U );
    LUMEN_CHECK_EQ( View[2], 3 );
}

LUMEN_TEST( Span_Constructor, FromTVector )
{
    TVector<Int32> Values{ 10, 20, 30 };

    TSpan<Int32> View{ Values };

    LUMEN_CHECK_EQ( View.size(), 3U );
    LUMEN_CHECK_TRUE( View.data() == Values.data() );
    LUMEN_CHECK_EQ( View[1], 20 );
}

LUMEN_TEST( Span_Constructor, DefaultIsEmpty )
{
    TSpan<Int32> View;

    LUMEN_CHECK_EQ( View.size(), 0U );
    LUMEN_CHECK_TRUE( View.empty() );
    LUMEN_CHECK_TRUE( View.data() == nullptr );
}

LUMEN_TEST( Span_Constructor, FixedExtentKeepsTheDeclaredSize )
{
    static constexpr Int32 FixedValues[4] = { 7, 8, 9, 10 };
    constexpr TSpan<const Int32, 4> FixedView{ FixedValues };

    static_assert( FixedView.extent == 4 );
    static_assert( FixedView.size() == 4 );
    static_assert( FixedView[0] == 7 );
    static_assert( FixedView[3] == 10 );
    static_assert( not FixedView.empty() );
}

/**
 * Span — Access
 */

LUMEN_TEST( Span_Access, ElementsAreReadableAndWritable )
{
    Int32 Values[3] = { 1, 2, 3 };

    TSpan<Int32> View{ Values };

    View[1] = 22;

    LUMEN_CHECK_EQ( Values[1], 22 );
    LUMEN_CHECK_EQ( View.front(), 1 );
    LUMEN_CHECK_EQ( View.back(), 3 );
    LUMEN_CHECK_EQ( View.size_bytes(), 12U );
}

LUMEN_TEST( Span_Access, SpanOfConstOnlyReads )
{
    static constexpr Int32 Values[3] = { 5, 6, 7 };
    const TSpan<const Int32> View{ Values };

    LUMEN_CHECK_EQ( View[1], 6 );
    LUMEN_CHECK_EQ( View.back(), 7 );

    static_assert( std::is_same_v<decltype( View[0] ), const Int32 &> );
    static_assert( View.extent == std::dynamic_extent );
}

LUMEN_TEST( Span_Access, EmptyFixedSizeSpan )
{
    static constexpr TSpan<Int32, 0> EmptyView{};

    static_assert( EmptyView.empty() );
    static_assert( EmptyView.extent == 0 );
}

/**
 * Span — Slicing
 */

LUMEN_TEST( Span_Slicing, SubViewsKeepTheUnderlyingStorage )
{
    Int32 Values[5] = { 1, 2, 3, 4, 5 };

    TSpan<Int32> View{ Values };

    TSpan<Int32, 2> FirstTwo = View.first<2>();
    TSpan<Int32> LastTwo     = View.last( 2 );
    TSpan<Int32> Middle      = View.subspan( 1, 3 );
    TSpan<Int32> Tail        = View.subspan( 4 );

    static_assert( FirstTwo.extent == 2 );

    LUMEN_CHECK_EQ( FirstTwo.front(), 1 );
    LUMEN_CHECK_EQ( LastTwo.back(), 5 );
    LUMEN_CHECK_EQ( Middle.size(), 3U );
    LUMEN_CHECK_EQ( Middle[0], 2 );
    LUMEN_CHECK_EQ( Tail.size(), 1U );
    LUMEN_CHECK_TRUE( Tail.data() == View.data() + 4 );
}

LUMEN_TEST( Span_Slicing, MutationsThroughSubViewsHitTheOriginal )
{
    Int32 Values[3] = { 0, 0, 0 };

    TSpan<Int32> View{ Values };

    View.first<2>()[1] = 9;
    View.last( 1 )[0]  = 8;

    LUMEN_CHECK_EQ( Values[0], 0 );
    LUMEN_CHECK_EQ( Values[1], 9 );
    LUMEN_CHECK_EQ( Values[2], 8 );
}

} // namespace Lumen
