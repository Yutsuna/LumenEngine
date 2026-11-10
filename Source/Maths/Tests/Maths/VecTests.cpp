/**
 * @file VecTests.cpp
 * @brief Exhaustive functional tests for Maths::TVec and NormalizePlane.
 */

#include <Maths/Vec.hpp>
#include <Tests/UnitTest.hpp>

#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

namespace Lumen
{

/**
 *  Compile-time invariants
 */

/** Dimensions: 1..4 accepted, 0 and 5 rejected. */
static_assert( Maths::CVecDimension<Float32, 1> );
static_assert( Maths::CVecDimension<Float32, 2> );
static_assert( Maths::CVecDimension<Float32, 3> );
static_assert( Maths::CVecDimension<Float32, 4> );
static_assert( !Maths::CVecDimension<Float32, 0> );
static_assert( !Maths::CVecDimension<Float32, 5> );

/** The standard aliases exist with the right underlying type. */
static_assert( std::is_same_v<Maths::FVec2f, Maths::TVec<Float32, 2>> );
static_assert( std::is_same_v<Maths::FVec3f, Maths::TVec<Float32, 3>> );
static_assert( std::is_same_v<Maths::FVec4f, Maths::TVec<Float32, 4>> );
static_assert( std::is_same_v<Maths::FVec2d, Maths::TVec<Float64, 2>> );
static_assert( std::is_same_v<Maths::FVec3d, Maths::TVec<Float64, 3>> );
static_assert( std::is_same_v<Maths::FVec4d, Maths::TVec<Float64, 4>> );
static_assert( std::is_same_v<Maths::FVec2i, Maths::TVec<Int32, 2>> );
static_assert( std::is_same_v<Maths::FVec3i, Maths::TVec<Int32, 3>> );
static_assert( std::is_same_v<Maths::FVec4i, Maths::TVec<Int32, 4>> );
static_assert( std::is_same_v<Maths::FVec2u, Maths::TVec<UInt32, 2>> );
static_assert( std::is_same_v<Maths::FVec3u, Maths::TVec<UInt32, 3>> );
static_assert( std::is_same_v<Maths::FVec4u, Maths::TVec<UInt32, 4>> );

/** Cross-type constructor is explicit. */
static_assert( !std::is_convertible_v<Maths::TVec<Float64, 3>, Maths::TVec<Float32, 3>> );
static_assert( std::is_constructible_v<Maths::TVec<Float32, 3>, const Maths::TVec<Float64, 3> &> );

/** Default-constructible and trivially copyable. */
static_assert( std::is_default_constructible_v<Maths::FVec3f> );
static_assert( std::is_trivially_copyable_v<Maths::FVec3f> );

/**
 *  Construction
 */

LUMEN_TEST( Vec_Ctor, DefaultIsZeroed )
{
    const Maths::FVec2f V2;
    const Maths::FVec3f V3;
    const Maths::FVec4f V4;

    /** TVecData initialises `Data[Dimension] = {}` so default-constructed */
    /** vectors must be zeroed. */
    LUMEN_CHECK_TRUE( Maths::IsEqual( V2.X, 0.0F ) and Maths::IsEqual( V2.Y, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( V3.X, 0.0F ) and Maths::IsEqual( V3.Y, 0.0F ) and Maths::IsEqual( V3.Z, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( V4.X, 0.0F ) and Maths::IsEqual( V4.Y, 0.0F ) and Maths::IsEqual( V4.Z, 0.0F ) and
                      Maths::IsEqual( V4.W, 0.0F ) );
}

LUMEN_TEST( Vec_Ctor, ScalarBroadcast )
{
    const Maths::FVec2f V2( 3.0F );
    const Maths::FVec3f V3( 7.0F );
    const Maths::FVec4f V4( -2.0F );

    LUMEN_CHECK_EQ( V2.X, 3.0F );
    LUMEN_CHECK_EQ( V2.Y, 3.0F );

    LUMEN_CHECK_EQ( V3.X, 7.0F );
    LUMEN_CHECK_EQ( V3.Y, 7.0F );
    LUMEN_CHECK_EQ( V3.Z, 7.0F );

    LUMEN_CHECK_EQ( V4.X, -2.0F );
    LUMEN_CHECK_EQ( V4.Y, -2.0F );
    LUMEN_CHECK_EQ( V4.Z, -2.0F );
    LUMEN_CHECK_EQ( V4.W, -2.0F );
}

LUMEN_TEST( Vec_Ctor, VariadicComponents )
{
    const Maths::FVec2f V2( 1.0F, 2.0F );
    const Maths::FVec3f V3( 1.0F, 2.0F, 3.0F );
    const Maths::FVec4f V4( 1.0F, 2.0F, 3.0F, 4.0F );

    LUMEN_CHECK_EQ( V2.X, 1.0F );
    LUMEN_CHECK_EQ( V2.Y, 2.0F );

    LUMEN_CHECK_EQ( V3.X, 1.0F );
    LUMEN_CHECK_EQ( V3.Y, 2.0F );
    LUMEN_CHECK_EQ( V3.Z, 3.0F );

    LUMEN_CHECK_EQ( V4.X, 1.0F );
    LUMEN_CHECK_EQ( V4.Y, 2.0F );
    LUMEN_CHECK_EQ( V4.Z, 3.0F );
    LUMEN_CHECK_EQ( V4.W, 4.0F );
}

LUMEN_TEST( Vec_Ctor, VariadicAcceptsIntegersWhenConvertible )
{
    /** ints are implicitly convertible to Float32 */
    const Maths::FVec3f V3( 1, 2, 3 );
    LUMEN_CHECK_EQ( V3.X, 1.0F );
    LUMEN_CHECK_EQ( V3.Y, 2.0F );
    LUMEN_CHECK_EQ( V3.Z, 3.0F );
}

LUMEN_TEST( Vec_Ctor, CrossTypeNarrowing )
{
    const Maths::FVec3d Source( 1.5, 2.5, 3.5 );
    const Maths::FVec3f Narrowed( Source );

    LUMEN_CHECK_EQ( Narrowed.X, 1.5F );
    LUMEN_CHECK_EQ( Narrowed.Y, 2.5F );
    LUMEN_CHECK_EQ( Narrowed.Z, 3.5F );
}

LUMEN_TEST( Vec_Ctor, CrossTypeWidening )
{
    const Maths::FVec3f Source( 0.25F, 0.5F, 0.75F );
    const Maths::FVec3d Widened( Source );

    LUMEN_CHECK_EQ( Widened.X, 0.25 );
    LUMEN_CHECK_EQ( Widened.Y, 0.5 );
    LUMEN_CHECK_EQ( Widened.Z, 0.75 );
}

LUMEN_TEST( Vec_Ctor, CrossTypeTruncatesFraction )
{
    /** Int32 -> Float32 is fine; Float32 -> Int32 truncates toward zero. */
    const Maths::FVec3f Source( 1.9F, -1.9F, 0.0F );
    const Maths::FVec3i Truncated( Source );

    LUMEN_CHECK_EQ( Truncated.X, 1 );
    LUMEN_CHECK_EQ( Truncated.Y, -1 );
    LUMEN_CHECK_EQ( Truncated.Z, 0 );
}

LUMEN_TEST( Vec_Ctor, CopyAndAssignment )
{
    const Maths::FVec3f A( 1.0F, 2.0F, 3.0F );
    Maths::FVec3f B( A );
    LUMEN_CHECK_TRUE( A == B );

    Maths::FVec3f C;
    C = A;
    LUMEN_CHECK_TRUE( A == C );

    Maths::FVec3f D;
    D = std::move( B );
    LUMEN_CHECK_TRUE( A == D );
}

/**
 *  Named components (X/Y/Z/W, R/G/B/A, S/T/P/Q, Width/Height)  — same storage
 */

LUMEN_TEST( Vec_Components, XAliasesRAndS )
{
    Maths::FVec3f V( 1.0F, 2.0F, 3.0F );

    LUMEN_CHECK_EQ( V.R, 1.0F );
    LUMEN_CHECK_EQ( V.S, 1.0F );

    V.R = 10.0F;
    LUMEN_CHECK_EQ( V.X, 10.0F );
    LUMEN_CHECK_EQ( V.S, 10.0F );

    V.S = 20.0F;
    LUMEN_CHECK_EQ( V.X, 20.0F );
    LUMEN_CHECK_EQ( V.R, 20.0F );
}

LUMEN_TEST( Vec_Components, YAliasesGAndT )
{
    Maths::FVec3f V( 1.0F, 2.0F, 3.0F );

    LUMEN_CHECK_EQ( V.G, 2.0F );
    LUMEN_CHECK_EQ( V.T, 2.0F );

    V.T = 99.0F;
    LUMEN_CHECK_EQ( V.Y, 99.0F );
}

LUMEN_TEST( Vec_Components, ZAliasesBAndP )
{
    Maths::FVec3f V( 1.0F, 2.0F, 3.0F );

    LUMEN_CHECK_EQ( V.B, 3.0F );
    LUMEN_CHECK_EQ( V.P, 3.0F );

    V.B = -1.0F;
    LUMEN_CHECK_EQ( V.Z, -1.0F );
    LUMEN_CHECK_EQ( V.P, -1.0F );
}

LUMEN_TEST( Vec_Components, WAliasesAAndQ )
{
    Maths::FVec4f V( 1.0F, 2.0F, 3.0F, 4.0F );

    LUMEN_CHECK_EQ( V.A, 4.0F );
    LUMEN_CHECK_EQ( V.Q, 4.0F );

    V.Q = 42.0F;
    LUMEN_CHECK_EQ( V.W, 42.0F );
    LUMEN_CHECK_EQ( V.A, 42.0F );
}

LUMEN_TEST( Vec_Components, Vec2WidthHeightAliasesXY )
{
    Maths::FVec2f V( 4.0F, 8.0F );
    LUMEN_CHECK_EQ( V.Width, 4.0F );
    LUMEN_CHECK_EQ( V.Height, 8.0F );

    V.Width = 100.0F;
    LUMEN_CHECK_EQ( V.X, 100.0F );
}

LUMEN_TEST( Vec_Components, DataArray )
{
    Maths::FVec4f V( 1.0F, 2.0F, 3.0F, 4.0F );
    LUMEN_CHECK_EQ( V.Data[0], 1.0F );
    LUMEN_CHECK_EQ( V.Data[1], 2.0F );
    LUMEN_CHECK_EQ( V.Data[2], 3.0F );
    LUMEN_CHECK_EQ( V.Data[3], 4.0F );

    V.Data[2] = 33.0F;
    LUMEN_CHECK_EQ( V.Z, 33.0F );
}

LUMEN_TEST( Vec_Components, SizeMatchesDimension )
{
    static_assert( sizeof( Maths::FVec2f ) == sizeof( Float32 ) * 2 );
    static_assert( sizeof( Maths::FVec3f ) == sizeof( Float32 ) * 3 );
    static_assert( sizeof( Maths::FVec4f ) == sizeof( Float32 ) * 4 );
    static_assert( sizeof( Maths::FVec2d ) == sizeof( Float64 ) * 2 );
}

/**
 *  Comparison
 */

LUMEN_TEST( Vec_Compare, EqualVectors )
{
    const Maths::FVec3f A( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f B( 1.0F, 2.0F, 3.0F );

    LUMEN_CHECK_TRUE( A == B );
    LUMEN_CHECK_FALSE( A != B );
}

LUMEN_TEST( Vec_Compare, DifferentInOneComponent )
{
    const Maths::FVec3f A( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f B( 1.0F, 2.0F, 3.0001F );

    LUMEN_CHECK_FALSE( A == B );
    LUMEN_CHECK_TRUE( A != B );
}

LUMEN_TEST( Vec_Compare, DifferentInEveryComponent )
{
    const Maths::FVec3f A( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f B( 4.0F, 5.0F, 6.0F );

    LUMEN_CHECK_FALSE( A == B );
    LUMEN_CHECK_TRUE( A != B );
}

LUMEN_TEST( Vec_Compare, NaNNotEqual )
{
    const Float32 Nan = std::numeric_limits<Float32>::quiet_NaN();
    const Maths::FVec3f A( Nan, 0.0F, 0.0F );
    const Maths::FVec3f B( Nan, 0.0F, 0.0F );

    /** operator== does per-component ==, and NaN != NaN. */
    LUMEN_CHECK_FALSE( A == B );
    LUMEN_CHECK_TRUE( A != B );
}

LUMEN_TEST( Vec_Compare, NegativeZeroEqualsPositiveZero )
{
    /** IEEE-754: -0.0 == 0.0 */
    const Maths::FVec2f A( -0.0F, 0.0F );
    const Maths::FVec2f B( 0.0F, -0.0F );
    LUMEN_CHECK_TRUE( A == B );
}

/**
 *  Arithmetic with another vector
 */

LUMEN_TEST( Vec_Arith, Add )
{
    const Maths::FVec3f A( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f B( 10.0F, 20.0F, 30.0F );
    const Maths::FVec3f R = A + B;

    LUMEN_CHECK_EQ( R.X, 11.0F );
    LUMEN_CHECK_EQ( R.Y, 22.0F );
    LUMEN_CHECK_EQ( R.Z, 33.0F );
}

LUMEN_TEST( Vec_Arith, Subtract )
{
    const Maths::FVec3f A( 10.0F, 20.0F, 30.0F );
    const Maths::FVec3f B( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f R = A - B;

    LUMEN_CHECK_EQ( R.X, 9.0F );
    LUMEN_CHECK_EQ( R.Y, 18.0F );
    LUMEN_CHECK_EQ( R.Z, 27.0F );
}

LUMEN_TEST( Vec_Arith, ComponentWiseMultiply )
{
    const Maths::FVec3f A( 2.0F, 3.0F, 4.0F );
    const Maths::FVec3f B( 5.0F, 6.0F, 7.0F );
    const Maths::FVec3f R = A * B;

    LUMEN_CHECK_EQ( R.X, 10.0F );
    LUMEN_CHECK_EQ( R.Y, 18.0F );
    LUMEN_CHECK_EQ( R.Z, 28.0F );
}

LUMEN_TEST( Vec_Arith, ComponentWiseDivide )
{
    const Maths::FVec3f A( 10.0F, 20.0F, 30.0F );
    const Maths::FVec3f B( 2.0F, 5.0F, 10.0F );
    const Maths::FVec3f R = A / B;

    LUMEN_CHECK_EQ( R.X, 5.0F );
    LUMEN_CHECK_EQ( R.Y, 4.0F );
    LUMEN_CHECK_EQ( R.Z, 3.0F );
}

LUMEN_TEST( Vec_Arith, AddSubDifferentSizes )
{
    const Maths::FVec2f A2( 1.0F, 2.0F );
    const Maths::FVec2f B2( 3.0F, 4.0F );
    LUMEN_CHECK_TRUE( ( A2 + B2 ) == Maths::FVec2f( 4.0F, 6.0F ) );

    const Maths::FVec4f A4( 1.0F, 2.0F, 3.0F, 4.0F );
    const Maths::FVec4f B4( 5.0F, 6.0F, 7.0F, 8.0F );
    LUMEN_CHECK_TRUE( ( A4 + B4 ) == Maths::FVec4f( 6.0F, 8.0F, 10.0F, 12.0F ) );
}

/**
 *  Arithmetic with a scalar
 */

LUMEN_TEST( Vec_Scalar, Add )
{
    const Maths::FVec3f V( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f R = V + 10.0F;

    LUMEN_CHECK_EQ( R.X, 11.0F );
    LUMEN_CHECK_EQ( R.Y, 12.0F );
    LUMEN_CHECK_EQ( R.Z, 13.0F );
}

LUMEN_TEST( Vec_Scalar, Subtract )
{
    const Maths::FVec3f V( 10.0F, 20.0F, 30.0F );
    const Maths::FVec3f R = V - 5.0F;

    LUMEN_CHECK_EQ( R.X, 5.0F );
    LUMEN_CHECK_EQ( R.Y, 15.0F );
    LUMEN_CHECK_EQ( R.Z, 25.0F );
}

LUMEN_TEST( Vec_Scalar, Multiply )
{
    const Maths::FVec3f V( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f R = V * 4.0F;

    LUMEN_CHECK_EQ( R.X, 4.0F );
    LUMEN_CHECK_EQ( R.Y, 8.0F );
    LUMEN_CHECK_EQ( R.Z, 12.0F );
}

LUMEN_TEST( Vec_Scalar, Divide )
{
    const Maths::FVec3f V( 4.0F, 8.0F, 12.0F );
    const Maths::FVec3f R = V / 2.0F;

    LUMEN_CHECK_EQ( R.X, 2.0F );
    LUMEN_CHECK_EQ( R.Y, 4.0F );
    LUMEN_CHECK_EQ( R.Z, 6.0F );
}

LUMEN_TEST( Vec_Scalar, DivideUsesReciprocal )
{
    const Maths::FVec3f V( 12.0F, 24.0F, 36.0F );
    const Maths::FVec3f R = V / 3.0F;

    LUMEN_CHECK_TRUE( Maths::IsEqual( R.X, 4.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Y, 8.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Z, 12.0F ) );
}

/**
 *  Compound assignment operators
 */

LUMEN_TEST( Vec_Compound, AddAssign )
{
    Maths::FVec3f V( 1.0F, 2.0F, 3.0F );
    V += Maths::FVec3f( 10.0F, 20.0F, 30.0F );
    LUMEN_CHECK_TRUE( V == Maths::FVec3f( 11.0F, 22.0F, 33.0F ) );
}

LUMEN_TEST( Vec_Compound, SubAssign )
{
    Maths::FVec3f V( 11.0F, 22.0F, 33.0F );
    V -= Maths::FVec3f( 1.0F, 2.0F, 3.0F );
    LUMEN_CHECK_TRUE( V == Maths::FVec3f( 10.0F, 20.0F, 30.0F ) );
}

LUMEN_TEST( Vec_Compound, MulAssign )
{
    Maths::FVec3f V( 2.0F, 3.0F, 4.0F );
    V *= Maths::FVec3f( 5.0F, 6.0F, 7.0F );
    LUMEN_CHECK_TRUE( V == Maths::FVec3f( 10.0F, 18.0F, 28.0F ) );
}

LUMEN_TEST( Vec_Compound, DivAssign )
{
    Maths::FVec3f V( 10.0F, 20.0F, 30.0F );
    V /= Maths::FVec3f( 2.0F, 5.0F, 10.0F );
    LUMEN_CHECK_TRUE( V == Maths::FVec3f( 5.0F, 4.0F, 3.0F ) );
}

LUMEN_TEST( Vec_Compound, Chained )
{
    Maths::FVec3f V( 1.0F, 2.0F, 3.0F );
    V += Maths::FVec3f( 1.0F, 1.0F, 1.0F );
    V *= Maths::FVec3f( 2.0F, 2.0F, 2.0F );
    LUMEN_CHECK_TRUE( V == Maths::FVec3f( 4.0F, 6.0F, 8.0F ) );
}

/**
 *  Unary negation
 */

LUMEN_TEST( Vec_Unary, Negate )
{
    const Maths::FVec3f V( 1.0F, -2.0F, 3.5F );
    const Maths::FVec3f R = -V;

    LUMEN_CHECK_EQ( R.X, -1.0F );
    LUMEN_CHECK_EQ( R.Y, 2.0F );
    LUMEN_CHECK_EQ( R.Z, -3.5F );
}

LUMEN_TEST( Vec_Unary, NegateZero )
{
    const Maths::FVec3f V( 0.0F );
    const Maths::FVec3f R = -V;

    /** -0.0F == 0.0F, so equality holds. */
    LUMEN_CHECK_TRUE( R == V );
}

LUMEN_TEST( Vec_Unary, DoubleNegationReturnsOriginal )
{
    const Maths::FVec3f V( 1.0F, -2.0F, 3.0F );
    LUMEN_CHECK_TRUE( -( -V ) == V );
}

/**
 *  Dot product
 */

LUMEN_TEST( Vec_Dot, OrthogonalIsZero )
{
    LUMEN_CHECK_EQ( Maths::FVec3f( 1.0F, 0.0F, 0.0F ).Dot( Maths::FVec3f( 0.0F, 1.0F, 0.0F ) ), 0.0F );
    LUMEN_CHECK_EQ( Maths::FVec3f( 1.0F, 0.0F, 0.0F ).Dot( Maths::FVec3f( 0.0F, 0.0F, 1.0F ) ), 0.0F );
}

LUMEN_TEST( Vec_Dot, SelfDotIsLengthSquared )
{
    const Maths::FVec3f V( 3.0F, 4.0F, 0.0F );
    LUMEN_CHECK_EQ( V.Dot( V ), 25.0F );
}

LUMEN_TEST( Vec_Dot, KnownValues )
{
    const Maths::FVec3f A( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f B( 4.0F, 5.0F, 6.0F );

    /** 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32 */
    LUMEN_CHECK_EQ( A.Dot( B ), 32.0F );
    LUMEN_CHECK_EQ( B.Dot( A ), 32.0F );
}

LUMEN_TEST( Vec_Dot, NegativeHandled )
{
    const Maths::FVec3f A( -1.0F, -2.0F, -3.0F );
    const Maths::FVec3f B( 1.0F, 2.0F, 3.0F );

    /** -(1+4+9) = -14 */
    LUMEN_CHECK_EQ( A.Dot( B ), -14.0F );
}

LUMEN_TEST( Vec_Dot, Vec2AndVec4 )
{
    LUMEN_CHECK_EQ( Maths::FVec2f( 3.0F, 4.0F ).Dot( Maths::FVec2f( 1.0F, 1.0F ) ), 7.0F );
    LUMEN_CHECK_EQ( Maths::FVec4f( 1.0F, 1.0F, 1.0F, 1.0F ).Dot( Maths::FVec4f( 1.0F, 2.0F, 3.0F, 4.0F ) ), 10.0F );
}

LUMEN_TEST( Vec_Dot, Symmetry )
{
    const Maths::FVec3f A( 1.5F, -2.5F, 3.5F );
    const Maths::FVec3f B( -4.5F, 5.5F, 6.5F );
    LUMEN_CHECK_TRUE( Maths::IsEqual( A.Dot( B ), B.Dot( A ) ) );
}

/**
 *  Cross product (Dim == 3 only)
 */

LUMEN_TEST( Vec_Cross, XCrossYEqualsZ )
{
    const Maths::FVec3f X( 1.0F, 0.0F, 0.0F );
    const Maths::FVec3f Y( 0.0F, 1.0F, 0.0F );
    const Maths::FVec3f R = X.Cross( Y );

    LUMEN_CHECK_TRUE( R == Maths::FVec3f( 0.0F, 0.0F, 1.0F ) );
}

LUMEN_TEST( Vec_Cross, YCrossXEqualsMinusZ )
{
    const Maths::FVec3f X( 1.0F, 0.0F, 0.0F );
    const Maths::FVec3f Y( 0.0F, 1.0F, 0.0F );
    const Maths::FVec3f R = Y.Cross( X );

    LUMEN_CHECK_TRUE( R == Maths::FVec3f( 0.0F, 0.0F, -1.0F ) );
}

LUMEN_TEST( Vec_Cross, ZCrossXEqualsY )
{
    const Maths::FVec3f X( 1.0F, 0.0F, 0.0F );
    const Maths::FVec3f Z( 0.0F, 0.0F, 1.0F );
    LUMEN_CHECK_TRUE( Z.Cross( X ) == Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );
}

LUMEN_TEST( Vec_Cross, ParallelGivesZero )
{
    const Maths::FVec3f A( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f B( 2.0F, 4.0F, 6.0F );
    const Maths::FVec3f R = A.Cross( B );

    LUMEN_CHECK_TRUE( R == Maths::FVec3f( 0.0F, 0.0F, 0.0F ) );
}

LUMEN_TEST( Vec_Cross, KnownValue )
{
    const Maths::FVec3f A( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f B( 4.0F, 5.0F, 6.0F );

    /** A x B = (2*6 - 3*5, 3*4 - 1*6, 1*5 - 2*4) = (12-15, 12-6, 5-8) = (-3, 6, -3) */
    const Maths::FVec3f R = A.Cross( B );
    LUMEN_CHECK_EQ( R.X, -3.0F );
    LUMEN_CHECK_EQ( R.Y, 6.0F );
    LUMEN_CHECK_EQ( R.Z, -3.0F );
}

LUMEN_TEST( Vec_Cross, AntiCommutative )
{
    const Maths::FVec3f A( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f B( 4.0F, 5.0F, 6.0F );
    LUMEN_CHECK_TRUE( A.Cross( B ) == -( B.Cross( A ) ) );
}

LUMEN_TEST( Vec_Cross, OrthogonalToBothOperands )
{
    const Maths::FVec3f A( 3.0F, -1.0F, 2.0F );
    const Maths::FVec3f B( 1.0F, 5.0F, -2.0F );
    const Maths::FVec3f C = A.Cross( B );

    LUMEN_CHECK_TRUE( Maths::IsEqual( C.Dot( A ), 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( C.Dot( B ), 0.0F ) );
}

/**
 *  Length, LengthSquared
 */

LUMEN_TEST( Vec_Length, Zero )
{
    LUMEN_CHECK_EQ( Maths::FVec3f( 0.0F ).Length(), 0.0F );
    LUMEN_CHECK_EQ( Maths::FVec3f( 0.0F ).LengthSquared(), 0.0F );
}

LUMEN_TEST( Vec_Length, UnitAxis )
{
    LUMEN_CHECK_EQ( Maths::FVec3f( 1.0F, 0.0F, 0.0F ).Length(), 1.0F );
    LUMEN_CHECK_EQ( Maths::FVec3f( 0.0F, 1.0F, 0.0F ).Length(), 1.0F );
    LUMEN_CHECK_EQ( Maths::FVec3f( 0.0F, 0.0F, 1.0F ).Length(), 1.0F );
}

LUMEN_TEST( Vec_Length, ThreeFourFive )
{
    const Maths::FVec2f V( 3.0F, 4.0F );
    LUMEN_CHECK_EQ( V.Length(), 5.0F );
    LUMEN_CHECK_EQ( V.LengthSquared(), 25.0F );
}

LUMEN_TEST( Vec_Length, LengthIsSqrtOfLengthSquared )
{
    const Maths::FVec3f V( 2.0F, 3.0F, 6.0F );
    LUMEN_CHECK_TRUE( Maths::IsEqual( V.Length(), std::sqrt( V.LengthSquared() ) ) );
}

LUMEN_TEST( Vec_Length, LengthSquaredIsDotSelf )
{
    const Maths::FVec3f V( 1.5F, -2.5F, 3.5F );
    LUMEN_CHECK_TRUE( Maths::IsEqual( V.LengthSquared(), V.Dot( V ) ) );
}

LUMEN_TEST( Vec_Length, Vec4 )
{
    const Maths::FVec4f V( 1.0F, 2.0F, 2.0F, 4.0F );
    /** 1 + 4 + 4 + 16 = 25, sqrt = 5 */
    LUMEN_CHECK_EQ( V.LengthSquared(), 25.0F );
    LUMEN_CHECK_EQ( V.Length(), 5.0F );
}

LUMEN_TEST( Vec_Length, Double )
{
    const Maths::FVec3d V( 3.0, 4.0, 12.0 );
    /** 9 + 16 + 144 = 169, sqrt = 13 */
    LUMEN_CHECK_TRUE( Maths::IsEqual( V.Length(), 13.0 ) );
}

/**
 *  Normalize
 */

LUMEN_TEST( Vec_Normalize, UnitStaysUnit )
{
    const Maths::FVec3f V( 1.0F, 0.0F, 0.0F );
    const Maths::FVec3f R = V.Normalize();

    LUMEN_CHECK_TRUE( R == V );
}

LUMEN_TEST( Vec_Normalize, ThreeFourFive )
{
    const Maths::FVec2f V( 3.0F, 4.0F );
    const Maths::FVec2f R = V.Normalize();

    LUMEN_CHECK_TRUE( Maths::IsEqual( R.X, 0.6F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Y, 0.8F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Length(), 1.0F, 1.0e-4F ) );
}

LUMEN_TEST( Vec_Normalize, ZeroReturnsZero )
{
    const Maths::FVec3f Z( 0.0F );
    const Maths::FVec3f R = Z.Normalize();

    /** Guarded by Len > 0 in the implementation. */
    LUMEN_CHECK_TRUE( R == Z );
}

LUMEN_TEST( Vec_Normalize, PreservesDirection )
{
    const Maths::FVec3f V( 2.0F, 0.0F, 0.0F );
    const Maths::FVec3f R = V.Normalize();

    LUMEN_CHECK_TRUE( Maths::IsEqual( R.X, 1.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Y, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Z, 0.0F ) );
}

LUMEN_TEST( Vec_Normalize, NegativeComponents )
{
    const Maths::FVec3f V( -3.0F, 0.0F, 4.0F );
    const Maths::FVec3f R = V.Normalize();

    LUMEN_CHECK_TRUE( Maths::IsEqual( R.X, -0.6F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Y, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Z, 0.8F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Length(), 1.0F, 1.0e-4F ) );
}

LUMEN_TEST( Vec_Normalize, Idempotent )
{
    const Maths::FVec3f V( 1.0F, 2.0F, 3.0F );
    const Maths::FVec3f N  = V.Normalize();
    const Maths::FVec3f NN = N.Normalize();

    LUMEN_CHECK_TRUE( Maths::IsEqual( N.X, NN.X ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( N.Y, NN.Y ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( N.Z, NN.Z ) );
}

LUMEN_TEST( Vec_Normalize, DoesNotModifyOriginal )
{
    const Maths::FVec3f V( 3.0F, 4.0F, 0.0F );
    static_cast<void>( V.Normalize() );

    LUMEN_CHECK_TRUE( V == Maths::FVec3f( 3.0F, 4.0F, 0.0F ) );
}

/**
 *  NormalizePlane (free function)
 */

LUMEN_TEST( Vec_NormalizePlane, UnitPlaneUnchanged )
{
    const Maths::FVec4f P( 1.0F, 0.0F, 0.0F, 5.0F );
    const Maths::FVec4f R = Maths::NormalizePlane( P );

    LUMEN_CHECK_TRUE( Maths::IsEqual( R.X, 1.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Y, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Z, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.W, 5.0F ) );
}

LUMEN_TEST( Vec_NormalizePlane, ScalesAllComponentsByInvMagnitude )
{
    /** Length of the xyz part is 5; each component is scaled by 1/5. */
    const Maths::FVec4f P( 3.0F, 4.0F, 0.0F, 10.0F );
    const Maths::FVec4f R = Maths::NormalizePlane( P );

    LUMEN_CHECK_TRUE( Maths::IsEqual( R.X, 0.6F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Y, 0.8F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Z, 0.0F ) );
    /** W is scaled too (this is the "distance" of the plane). */
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.W, 2.0F ) );
}

LUMEN_TEST( Vec_NormalizePlane, ZeroPlaneUnchanged )
{
    /** MagSq <= eps^2 → returns the plane untouched. */
    const Maths::FVec4f P( 0.0F, 0.0F, 0.0F, 42.0F );
    const Maths::FVec4f R = Maths::NormalizePlane( P );

    LUMEN_CHECK_TRUE( Maths::IsEqual( R.X, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Y, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.Z, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.W, 42.0F ) );
}

LUMEN_TEST( Vec_NormalizePlane, NearlyZeroPlaneUnchanged )
{
    /** Below epsilon → not renormalised. */
    const Float32 Tiny = std::numeric_limits<Float32>::epsilon() * 0.5F;
    const Maths::FVec4f P( Tiny, Tiny, Tiny, 1.0F );
    const Maths::FVec4f R = Maths::NormalizePlane( P );

    LUMEN_CHECK_TRUE( Maths::IsEqual( R.X, Tiny ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R.W, 1.0F ) );
}

LUMEN_TEST( Vec_NormalizePlane, ResultHasUnitNormal )
{
    const Maths::FVec4f P( 3.0F, 4.0F, 12.0F, 7.0F );
    const Maths::FVec4f R = Maths::NormalizePlane( P );

    const Maths::FVec3f N( R.X, R.Y, R.Z );
    LUMEN_CHECK_TRUE( Maths::IsEqual( N.Length(), 1.0F, 1.0e-4F ) );
}

/**
 *  Type promotion / integer vectors
 */

LUMEN_TEST( Vec_Ints, AddSub )
{
    const Maths::FVec3i A( 10, 20, 30 );
    const Maths::FVec3i B( 1, 2, 3 );

    LUMEN_CHECK_TRUE( ( A + B ) == Maths::FVec3i( 11, 22, 33 ) );
    LUMEN_CHECK_TRUE( ( A - B ) == Maths::FVec3i( 9, 18, 27 ) );
}

LUMEN_TEST( Vec_Ints, MulDivTruncate )
{
    const Maths::FVec3i A( 7, 8, 9 );
    const Maths::FVec3i B( 2, 3, 4 );

    /** Integer division truncates toward zero. */
    LUMEN_CHECK_TRUE( ( A / B ) == Maths::FVec3i( 3, 2, 2 ) );
    LUMEN_CHECK_TRUE( ( A * B ) == Maths::FVec3i( 14, 24, 36 ) );
}

LUMEN_TEST( Vec_Ints, ScalarArithmetic )
{
    const Maths::FVec3i A( 1, 2, 3 );
    LUMEN_CHECK_TRUE( ( A * 3 ) == Maths::FVec3i( 3, 6, 9 ) );
    LUMEN_CHECK_TRUE( ( A + 1 ) == Maths::FVec3i( 2, 3, 4 ) );
}

LUMEN_TEST( Vec_Ints, LengthSquared )
{
    LUMEN_CHECK_EQ( Maths::FVec3i( 1, 2, 3 ).LengthSquared(), 14 );
}

LUMEN_TEST( Vec_Ints, NormalizeProducesZeroForUnitSizedVecs )
{
    /** Int32 normalize uses integer division: 3 / 3 = 1, but 4 / 5 = 0 ... */
    /** This test locks in the *actual* current behaviour of integer vectors. */
    const Maths::FVec3i V( 3, 0, 0 );
    const Maths::FVec3i R = V.Normalize();

    LUMEN_CHECK_EQ( R.X, 1 );
    LUMEN_CHECK_EQ( R.Y, 0 );
    LUMEN_CHECK_EQ( R.Z, 0 );
}

/**
 *  Compile-time arithmetic (constexpr sanity)
 */

LUMEN_TEST( Vec_Constexpr, ConstructionAndOps )
{
    constexpr Maths::FVec3f A( 1.0F, 2.0F, 3.0F );
    constexpr Maths::FVec3f B( 4.0F, 5.0F, 6.0F );
    constexpr Maths::FVec3f Sum  = A + B;
    constexpr Maths::FVec3f Diff = B - A;
    constexpr Float32 Dot        = A.Dot( B );
    constexpr Float32 LenSq      = A.LengthSquared();

    static_assert( Sum.Data[0] == 5.0F and Sum.Data[1] == 7.0F and Sum.Data[2] == 9.0F );
    static_assert( Diff.Data[0] == 3.0F and Diff.Data[1] == 3.0F and Diff.Data[2] == 3.0F );
    static_assert( Dot == 32.0F );
    static_assert( LenSq == 14.0F );
}

LUMEN_TEST( Vec_Constexpr, Cross )
{
    constexpr Maths::FVec3f X( 1.0F, 0.0F, 0.0F );
    constexpr Maths::FVec3f Y( 0.0F, 1.0F, 0.0F );
    constexpr Maths::FVec3f Z = X.Cross( Y );

    static_assert( Z.Data[0] == 0.0F and Z.Data[1] == 0.0F and Z.Data[2] == 1.0F );
}

} // namespace Lumen
