/**
 * @file MatrixTests.cpp
 * @brief Exhaustive functional tests for Maths::TMatrix and TransformAABB.
 */

#include <Maths/Macros.hpp>
#include <Maths/Matrix.hpp>
#include <Maths/Vec.hpp>
#include <Tests/UnitTest.hpp>

#include <cmath>
#include <type_traits>

namespace Lumen
{

namespace
{

    template <typename Type, USize R, USize C>
    void CheckRowAndColumnAccess ( const Maths::TMatrix<Type, R, C> &M, const Type Tolerance )
    {
        for ( USize Col = 0; Col < C; ++Col )
        {
            for ( USize Row = 0; Row < R; ++Row )
            {
                /** Build a row vector and compare against M.GetRow(Row)[Col]. */
                auto RowVec = M.GetRow( Row );
                LUMEN_CHECK_TRUE( std::fabs( RowVec.Data[Col] - M[Col].Data[Row] ) <= Tolerance );
            }
        }
    }

} // namespace

/**
 *  Compile-time invariants
 */

/** Dimensions accepted: any non-zero up to 4. */
static_assert( Maths::Concepts::CMatrixDimension<1, 1> );
static_assert( Maths::Concepts::CMatrixDimension<4, 4> );
static_assert( Maths::Concepts::CMatrixDimension<2, 3> );
static_assert( !Maths::Concepts::CMatrixDimension<0, 1> );
static_assert( !Maths::Concepts::CMatrixDimension<1, 0> );
static_assert( !Maths::Concepts::CMatrixDimension<5, 5> );
static_assert( !Maths::Concepts::CMatrixDimension<1, 5> );

static_assert( Maths::Concepts::CSquareMatrix<2, 2> );
static_assert( !Maths::Concepts::CSquareMatrix<2, 3> );
static_assert( Maths::Concepts::CSquareMatrix<4, 4> );

/** Factory methods only exist for square 4×4. */
static_assert( requires { Maths::FMat4x4f::Identity(); } );
static_assert( requires { Maths::FMat4x4f::Translate( Maths::FVec3f{} ); } );
static_assert( requires { Maths::FMat4x4f::Perspective( 1.0F, 1.0F, 0.1F, 1000.0F ); } );
static_assert( requires { Maths::FMat4x4f::LookAt( Maths::FVec3f{}, Maths::FVec3f{}, Maths::FVec3f{} ); } );

/** Aliases */
static_assert( std::is_same_v<Maths::FMat2x2f, Maths::TMatrix<Float32, 2, 2>> );
static_assert( std::is_same_v<Maths::FMat3x3f, Maths::TMatrix<Float32, 3, 3>> );
static_assert( std::is_same_v<Maths::FMat4x4f, Maths::TMatrix<Float32, 4, 4>> );
static_assert( std::is_same_v<Maths::FMat2x2d, Maths::TMatrix<Float64, 2, 2>> );
static_assert( std::is_same_v<Maths::FMat3x3d, Maths::TMatrix<Float64, 3, 3>> );
static_assert( std::is_same_v<Maths::FMat4x4d, Maths::TMatrix<Float64, 4, 4>> );

/**
 *  Construction
 */

LUMEN_TEST( Matrix_Ctor, DefaultIsUninitialised_DoNotAssertValue )
{
    /** The default ctor is `= default` and does not zero-init Data[]. */
    /** We only assert that the object is constructible and usable. */
    Maths::FMat4x4f M;

    /** Writable, readable (values unspecified). */
    M[0].Data[0] = 1.0F;
    LUMEN_CHECK_EQ( M[0].Data[0], 1.0F );
}

LUMEN_TEST( Matrix_Ctor, ScalarDiagonal )
{
    const Maths::FMat3x3f M( 2.0F );
    for ( USize Col = 0; Col < 3; ++Col )
    {
        for ( USize Row = 0; Row < 3; ++Row )
        {
            const Float32 Expected = ( Col == Row ) ? 2.0F : 0.0F;
            LUMEN_CHECK_EQ( M[Col].Data[Row], Expected );
        }
    }
}

LUMEN_TEST( Matrix_Ctor, ScalarZero )
{
    const Maths::FMat4x4f M( 0.0F );
    for ( USize Col = 0; Col < 4; ++Col )
    {
        for ( USize Row = 0; Row < 4; ++Row )
        {
            LUMEN_CHECK_EQ( M[Col].Data[Row], 0.0F );
        }
    }
}

LUMEN_TEST( Matrix_Ctor, FromColumnArray )
{
    const Maths::TVec<Float32, 2> Columns[2] = { Maths::TVec<Float32, 2>( 1.0F, 2.0F ), Maths::TVec<Float32, 2>( 3.0F, 4.0F ) };
    const Maths::TMatrix<Float32, 2, 2> M( Columns );

    LUMEN_CHECK_EQ( M[0].Data[0], 1.0F );
    LUMEN_CHECK_EQ( M[0].Data[1], 2.0F );
    LUMEN_CHECK_EQ( M[1].Data[0], 3.0F );
    LUMEN_CHECK_EQ( M[1].Data[1], 4.0F );
}

LUMEN_TEST( Matrix_Ctor, CopyAndMove )
{
    Maths::FMat2x2f A( 3.0F );
    Maths::FMat2x2f B( A );
    LUMEN_CHECK_TRUE( A == B );

    Maths::FMat2x2f C;
    C = A;
    LUMEN_CHECK_TRUE( A == C );

    Maths::FMat2x2f D;
    D = std::move( B );
    LUMEN_CHECK_TRUE( A == D );
}

/**
 *  Column access (operator[])
 */

LUMEN_TEST( Matrix_Column, MutableAccess )
{
    Maths::FMat3x3f M( 0.0F );

    M[1].Data[2] = 42.0F;
    LUMEN_CHECK_EQ( M[1].Data[2], 42.0F );

    M[2].X = 1.0F;
    LUMEN_CHECK_EQ( M[2].Data[0], 1.0F );
}

LUMEN_TEST( Matrix_Column, ConstAccess )
{
    Maths::FMat3x3f M( 7.0F );
    const auto &CM = M;
    LUMEN_CHECK_EQ( CM[0].Data[0], 7.0F );
    LUMEN_CHECK_EQ( CM[1].Data[1], 7.0F );
    LUMEN_CHECK_EQ( CM[2].Data[2], 7.0F );
}

/**
 *  GetRow
 */

LUMEN_TEST( Matrix_GetRow, RowOfIdentity )
{
    const Maths::FMat3x3f M( 1.0F );
    const Maths::FVec3f R0 = M.GetRow( 0 );
    const Maths::FVec3f R1 = M.GetRow( 1 );
    const Maths::FVec3f R2 = M.GetRow( 2 );

    LUMEN_CHECK_TRUE( R0 == Maths::FVec3f( 1.0F, 0.0F, 0.0F ) );
    LUMEN_CHECK_TRUE( R1 == Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );
    LUMEN_CHECK_TRUE( R2 == Maths::FVec3f( 0.0F, 0.0F, 1.0F ) );
}

LUMEN_TEST( Matrix_GetRow, RowOfCustomMatrix )
{
    /** Column-major fill. */
    const Maths::TVec<Float32, 2> C0[2] = { Maths::TVec<Float32, 2>( 1.0F, 2.0F ), Maths::TVec<Float32, 2>( 3.0F, 4.0F ) };
    const Maths::TMatrix<Float32, 2, 2> M( C0 );

    const auto R0 = M.GetRow( 0 );
    const auto R1 = M.GetRow( 1 );

    /** row 0 = { M[0].X, M[1].X } = { 1, 3 } */
    LUMEN_CHECK_EQ( R0.Data[0], 1.0F );
    LUMEN_CHECK_EQ( R0.Data[1], 3.0F );

    /** row 1 = { M[0].Y, M[1].Y } = { 2, 4 } */
    LUMEN_CHECK_EQ( R1.Data[0], 2.0F );
    LUMEN_CHECK_EQ( R1.Data[1], 4.0F );
}

LUMEN_TEST( Matrix_GetRow, RowOfNonSquareMatrix )
{
    Maths::FMat3x4d M( 0.0 );
    M[0].Data[0] = 1.0;
    M[0].Data[1] = 2.0;
    M[0].Data[2] = 3.0;
    M[1].Data[0] = 4.0;
    M[1].Data[1] = 5.0;
    M[1].Data[2] = 6.0;
    M[2].Data[0] = 7.0;
    M[2].Data[1] = 8.0;
    M[2].Data[2] = 9.0;

    /** Row 1 of a 3×4 matrix is a 4-vector. */
    const auto Row = M.GetRow( 1 );
    LUMEN_CHECK_EQ( Row.Data[0], 2.0 );
    LUMEN_CHECK_EQ( Row.Data[1], 5.0 );
    LUMEN_CHECK_EQ( Row.Data[2], 8.0 );
}

/**
 *  Equality
 */

LUMEN_TEST( Matrix_Compare, IdentityEqualsIdentity )
{
    LUMEN_CHECK_TRUE( Maths::FMat4x4f::Identity() == Maths::FMat4x4f::Identity() );
}

LUMEN_TEST( Matrix_Compare, DifferentNotEqual )
{
    LUMEN_CHECK_TRUE( Maths::FMat4x4f::Identity() != Maths::FMat4x4f( 0.0F ) );
}

LUMEN_TEST( Matrix_Compare, OneElementDiffers )
{
    Maths::FMat3x3f A( 1.0F );
    Maths::FMat3x3f B( 1.0F );
    B[0].Data[1] = 0.001F;

    LUMEN_CHECK_TRUE( A != B );
}

/**
 *  Factories — Identity
 */

LUMEN_TEST( Matrix_Identity, DiagonalIsOne )
{
    const auto I = Maths::FMat4x4f::Identity();
    for ( USize Col = 0; Col < 4; ++Col )
    {
        for ( USize Row = 0; Row < 4; ++Row )
        {
            const Float32 Expected = ( Col == Row ) ? 1.0F : 0.0F;
            LUMEN_CHECK_EQ( I[Col].Data[Row], Expected );
        }
    }
}

LUMEN_TEST( Matrix_Identity, IdentityTimesMatrixIsMatrix )
{
    const auto I = Maths::FMat4x4f::Identity();
    const auto M = Maths::FMat4x4f::Translate( Maths::FVec3f( 1.0F, 2.0F, 3.0F ) );

    LUMEN_CHECK_TRUE( Maths::IsEqual( I * M, M ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( M * I, M ) );
}

/**
 *  Factories — Translate / Scale
 */

LUMEN_TEST( Matrix_Translate, LastColumnHoldsTranslation )
{
    const auto M = Maths::FMat4x4f::Translate( Maths::FVec3f( 10.0F, 20.0F, 30.0F ) );

    LUMEN_CHECK_EQ( M[3].Data[0], 10.0F );
    LUMEN_CHECK_EQ( M[3].Data[1], 20.0F );
    LUMEN_CHECK_EQ( M[3].Data[2], 30.0F );
    LUMEN_CHECK_EQ( M[3].Data[3], 1.0F );
}

LUMEN_TEST( Matrix_Translate, LinearPartIsIdentity )
{
    const auto M = Maths::FMat4x4f::Translate( Maths::FVec3f( 5.0F, 6.0F, 7.0F ) );

    LUMEN_CHECK_EQ( M[0].Data[0], 1.0F );
    LUMEN_CHECK_EQ( M[1].Data[1], 1.0F );
    LUMEN_CHECK_EQ( M[2].Data[2], 1.0F );

    LUMEN_CHECK_EQ( M[0].Data[1], 0.0F );
    LUMEN_CHECK_EQ( M[1].Data[0], 0.0F );
    LUMEN_CHECK_EQ( M[2].Data[0], 0.0F );
}

LUMEN_TEST( Matrix_Translate, ZeroIsIdentity )
{
    const auto M = Maths::FMat4x4f::Translate( Maths::FVec3f( 0.0F ) );
    LUMEN_CHECK_TRUE( M == Maths::FMat4x4f::Identity() );
}

LUMEN_TEST( Matrix_Scale, DiagonalHasScale )
{
    const auto M = Maths::FMat4x4f::Scale( Maths::FVec3f( 2.0F, 3.0F, 4.0F ) );

    LUMEN_CHECK_EQ( M[0].Data[0], 2.0F );
    LUMEN_CHECK_EQ( M[1].Data[1], 3.0F );
    LUMEN_CHECK_EQ( M[2].Data[2], 4.0F );
    LUMEN_CHECK_EQ( M[3].Data[3], 1.0F );
}

LUMEN_TEST( Matrix_Scale, OffDiagonalsAreZero )
{
    const auto M = Maths::FMat4x4f::Scale( Maths::FVec3f( 2.0F, 3.0F, 4.0F ) );
    for ( USize Col = 0; Col < 4; ++Col )
    {
        for ( USize Row = 0; Row < 4; ++Row )
        {
            if ( Col != Row )
            {
                LUMEN_CHECK_EQ( M[Col].Data[Row], 0.0F );
            }
        }
    }
}

/**
 *  Factories — Rotations
 */

LUMEN_TEST( Matrix_RotateX, ZeroIsIdentity )
{
    const auto M = Maths::FMat4x4f::RotateX( 0.0F );
    LUMEN_CHECK_TRUE( Maths::IsEqual( M, Maths::FMat4x4f::Identity() ) );
}

LUMEN_TEST( Matrix_RotateX, QuarterTurnRotatesYToZ )
{
    const Float32 HalfPi = Maths::HalfPi<Float32>;
    const auto M         = Maths::FMat4x4f::RotateX( HalfPi );

    const Maths::FVec3f YAxis( 0.0F, 1.0F, 0.0F );
    /** Column-major matrix * column vector = sum over columns scaled by v[col]. */
    const Maths::FVec3f YRotated( M[0].X * YAxis.X + M[1].X * YAxis.Y + M[2].X * YAxis.Z + M[3].X,
                                  M[0].Y * YAxis.X + M[1].Y * YAxis.Y + M[2].Y * YAxis.Z + M[3].Y,
                                  M[0].Z * YAxis.X + M[1].Z * YAxis.Y + M[2].Z * YAxis.Z + M[3].Z );

    LUMEN_CHECK_TRUE( std::fabs( YRotated.X ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( YRotated.Y ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( YRotated.Z - 1.0F ) < 1.0e-4F );
}

LUMEN_TEST( Matrix_RotateY, QuarterTurnRotatesZToX )
{
    const Float32 HalfPi = Maths::HalfPi<Float32>;
    const auto M         = Maths::FMat4x4f::RotateY( HalfPi );

    /** Z axis -> X axis with the given layout (Y rotation: [cos, -sin; sin, cos] */
    /** placed at cols 0 and 2). Verify by inspecting entries. */
    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[0] ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[2] - 1.0F ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[2].Data[0] + 1.0F ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[2].Data[2] ) < 1.0e-4F );
}

LUMEN_TEST( Matrix_RotateZ, QuarterTurnRotatesXToY )
{
    const Float32 HalfPi = Maths::HalfPi<Float32>;
    const auto M         = Maths::FMat4x4f::RotateZ( HalfPi );

    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[0] ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[1] - 1.0F ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[1].Data[0] + 1.0F ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[1].Data[1] ) < 1.0e-4F );
}

LUMEN_TEST( Matrix_Rotations, HalfTurnIsNegationOfIdentityOnPlane )
{
    /** Rotate by pi about Z: cos = -1, sin = 0 */
    const Float32 Pi = Maths::Pi<Float32>;
    const auto M     = Maths::FMat4x4f::RotateZ( Pi );

    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[0] + 1.0F ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[1].Data[1] + 1.0F ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[1] ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[1].Data[0] ) < 1.0e-4F );
}

/**
 *  Factories — Perspective
 */

LUMEN_TEST( Matrix_Perspective, EntryLayout )
{
    const Float32 Fov    = Maths::HalfPi<Float32>;
    const Float32 Aspect = 16.0F / 9.0F;
    const Float32 NearZ  = 0.1F;
    const Float32 FarZ   = 1000.0F;

    const auto M = Maths::FMat4x4f::Perspective( Fov, Aspect, NearZ, FarZ );

    const Float32 TanHalf = std::tan( Fov * 0.5F );

    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[0] - 1.0F / ( Aspect * TanHalf ) ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[1].Data[1] - ( -1.0F / TanHalf ) ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[2].Data[2] - ( FarZ / ( FarZ - NearZ ) ) ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[2].Data[3] - 1.0F ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[3].Data[2] - ( -( FarZ * NearZ ) / ( FarZ - NearZ ) ) ) < 1.0e-4F );

    /** Untouched entries */
    LUMEN_CHECK_EQ( M[0].Data[1], 0.0F );
    LUMEN_CHECK_EQ( M[1].Data[0], 0.0F );
    LUMEN_CHECK_EQ( M[3].Data[3], 0.0F );
}

/**
 *  Factories — Orthographic
 */

LUMEN_TEST( Matrix_Orthographic, EntryLayout )
{
    const auto M = Maths::FMat4x4f::Orthographic( 2.0F, 2.0F, 0.1F, 100.0F );

    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[0] - 1.0F ) < 1.0e-4F ); /** 2/2 */
    LUMEN_CHECK_TRUE( std::fabs( M[1].Data[1] + 1.0F ) < 1.0e-4F ); /** -2/2 */
    LUMEN_CHECK_TRUE( std::fabs( M[2].Data[2] - ( 1.0F / 99.9F ) ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[3].Data[2] + ( 0.1F / 99.9F ) ) < 1.0e-4F );
    LUMEN_CHECK_EQ( M[3].Data[3], 1.0F );
}

/**
 *  Factories — LookAt
 */

LUMEN_TEST( Matrix_LookAt, FromOriginTowardsNegativeZ )
{
    const Maths::FVec3f Eye( 0.0F, 0.0F, 0.0F );
    const Maths::FVec3f Target( 0.0F, 0.0F, -1.0F );
    const Maths::FVec3f Up( 0.0F, 1.0F, 0.0F );

    const auto M = Maths::FMat4x4f::LookAt( Eye, Target, Up );

    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[0] + 1.0F ) < 1.0e-4F ); /** R.X */
    LUMEN_CHECK_TRUE( std::fabs( M[1].Data[0] ) < 1.0e-4F );        /** R.Y */
    LUMEN_CHECK_TRUE( std::fabs( M[2].Data[0] ) < 1.0e-4F );        /** R.Z */

    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[1] ) < 1.0e-4F );        /** U.X */
    LUMEN_CHECK_TRUE( std::fabs( M[1].Data[1] - 1.0F ) < 1.0e-4F ); /** U.Y */
    LUMEN_CHECK_TRUE( std::fabs( M[2].Data[1] ) < 1.0e-4F );        /** U.Z */

    LUMEN_CHECK_TRUE( std::fabs( M[0].Data[2] ) < 1.0e-4F );        /** F.X */
    LUMEN_CHECK_TRUE( std::fabs( M[1].Data[2] ) < 1.0e-4F );        /** F.Y */
    LUMEN_CHECK_TRUE( std::fabs( M[2].Data[2] + 1.0F ) < 1.0e-4F ); /** F.Z */

    /** Translation part is zero (eye at origin). */
    LUMEN_CHECK_TRUE( std::fabs( M[3].Data[0] ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[3].Data[1] ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( M[3].Data[2] ) < 1.0e-4F );
}

LUMEN_TEST( Matrix_LookAt, EyeAtOriginLookingNegativeZIsRigidTransform )
{
    const auto M = Maths::FMat4x4f::LookAt( Maths::FVec3f( 0.0F, 0.0F, 0.0F ), Maths::FVec3f( 0.0F, 0.0F, -1.0F ),
                                            Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );

    /** Upper 3×3 must be orthonormal: length of each column is 1 and mutually orthogonal. */
    const Maths::FVec3f C0( M[0].Data[0], M[0].Data[1], M[0].Data[2] );
    const Maths::FVec3f C1( M[1].Data[0], M[1].Data[1], M[1].Data[2] );
    const Maths::FVec3f C2( M[2].Data[0], M[2].Data[1], M[2].Data[2] );

    LUMEN_CHECK_TRUE( std::fabs( C0.Length() - 1.0F ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( C1.Length() - 1.0F ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( C2.Length() - 1.0F ) < 1.0e-4F );

    LUMEN_CHECK_TRUE( std::fabs( C0.Dot( C1 ) ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( C0.Dot( C2 ) ) < 1.0e-4F );
    LUMEN_CHECK_TRUE( std::fabs( C1.Dot( C2 ) ) < 1.0e-4F );
}

LUMEN_TEST( Matrix_LookAt, NonOriginEyeBakesTranslation )
{
    const Maths::FVec3f Eye( 5.0F, 0.0F, 0.0F );
    const auto M = Maths::FMat4x4f::LookAt( Eye, Maths::FVec3f( 5.0F, 0.0F, -1.0F ), Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );

    /** Translation column must not be zero anymore. */
    const Float32 Mag = std::fabs( M[3].Data[0] ) + std::fabs( M[3].Data[1] ) + std::fabs( M[3].Data[2] );
    LUMEN_CHECK_TRUE( Mag > 0.5F );
}

/**
 *  operator* — matrix × matrix
 */

LUMEN_TEST( Matrix_Mul, FourByFour_Float32_SIMDPath_IdentityIsNoOp )
{
    const auto I = Maths::FMat4x4f::Identity();
    const auto M = Maths::FMat4x4f::Translate( Maths::FVec3f( 1.0F, 2.0F, 3.0F ) );

    const auto R1 = I * M;
    const auto R2 = M * I;

    LUMEN_CHECK_TRUE( Maths::IsEqual( R1, M ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( R2, M ) );
}

LUMEN_TEST( Matrix_Mul, FourByFour_Float32_SIMDPath_KnownValue )
{
    /** A = translate(1,2,3), B = identity → A*B = A. */
    const auto A = Maths::FMat4x4f::Translate( Maths::FVec3f( 1.0F, 2.0F, 3.0F ) );
    const auto B = Maths::FMat4x4f::Identity();

    const auto R = A * B;

    LUMEN_CHECK_TRUE( std::fabs( R[3].Data[0] - 1.0F ) < 1.0e-5F );
    LUMEN_CHECK_TRUE( std::fabs( R[3].Data[1] - 2.0F ) < 1.0e-5F );
    LUMEN_CHECK_TRUE( std::fabs( R[3].Data[2] - 3.0F ) < 1.0e-5F );
}

LUMEN_TEST( Matrix_Mul, FourByFour_Float32_SIMDPath_Associative )
{
    const auto A = Maths::FMat4x4f::Translate( Maths::FVec3f( 1.0F, 2.0F, 3.0F ) );
    const auto B = Maths::FMat4x4f::Scale( Maths::FVec3f( 2.0F, 3.0F, 4.0F ) );
    const auto C = Maths::FMat4x4f::RotateZ( Maths::HalfPi<Float32> * 0.5F );

    const auto ABC1 = A * ( B * C );
    const auto ABC2 = ( A * B ) * C;

    LUMEN_CHECK_TRUE( Maths::IsEqual( ABC1, ABC2, 1.0e-4F ) );
}

LUMEN_TEST( Matrix_Mul, TwoByTwo_Float64_ScalarPath )
{
    /** Float64 avoids the SIMD branch (which would misbehave on a 2×2). */
    Maths::FMat2x2d A( 0.0 );
    A[0].Data[0] = 1.0;
    A[0].Data[1] = 2.0;
    A[1].Data[0] = 3.0;
    A[1].Data[1] = 4.0;

    Maths::FMat2x2d B( 0.0 );
    B[0].Data[0] = 5.0;
    B[0].Data[1] = 6.0;
    B[1].Data[0] = 7.0;
    B[1].Data[1] = 8.0;

    /**
     * Column-major multiplication: C[c][r] = sum_k A[k][r] * B[c][k]
     * C[0][0] = 1*5 + 3*6 = 5 + 18 = 23
     * C[0][1] = 2*5 + 4*6 = 10 + 24 = 34
     * C[1][0] = 1*7 + 3*8 = 7 + 24 = 31
     * C[1][1] = 2*7 + 4*8 = 14 + 32 = 46
     */
    const Maths::FMat2x2d R = A * B;

    LUMEN_CHECK_EQ( R[0].Data[0], 23.0 );
    LUMEN_CHECK_EQ( R[0].Data[1], 34.0 );
    LUMEN_CHECK_EQ( R[1].Data[0], 31.0 );
    LUMEN_CHECK_EQ( R[1].Data[1], 46.0 );
}

LUMEN_TEST( Matrix_Mul, TwoByThree_Times_ThreeByTwo_Float64 )
{
    Maths::FMat2x3d A( 0.0 );
    A[0].Data[0] = 1.0;
    A[0].Data[1] = 2.0;
    A[1].Data[0] = 3.0;
    A[1].Data[1] = 4.0;
    A[2].Data[0] = 5.0;
    A[2].Data[1] = 6.0;

    Maths::FMat3x2d B( 0.0 );
    B[0].Data[0] = 7.0;
    B[0].Data[1] = 8.0;
    B[0].Data[2] = 9.0;
    B[1].Data[0] = 10.0;
    B[1].Data[1] = 11.0;
    B[1].Data[2] = 12.0;

    /** Result is 2×2. */
    const Maths::FMat2x2d R = A * B;

    /** R[0][0] = 1*7 + 3*8 + 5*9 = 7 + 24 + 45 = 76 */
    /** R[0][1] = 2*7 + 4*8 + 6*9 = 14 + 32 + 54 = 100 */
    /** R[1][0] = 1*10 + 3*11 + 5*12 = 10 + 33 + 60 = 103 */
    /** R[1][1] = 2*10 + 4*11 + 6*12 = 20 + 44 + 72 = 136 */
    LUMEN_CHECK_EQ( R[0].Data[0], 76.0 );
    LUMEN_CHECK_EQ( R[0].Data[1], 100.0 );
    LUMEN_CHECK_EQ( R[1].Data[0], 103.0 );
    LUMEN_CHECK_EQ( R[1].Data[1], 136.0 );
}

LUMEN_TEST( Matrix_Mul, ThreeByThree_Float64_KnownValue )
{
    Maths::FMat3x3d A( 0.0 );
    Maths::FMat3x3d B( 0.0 );

    /** A = [[1,2,3],[4,5,6],[7,8,9]] in column-major. */
    A[0].Data[0] = 1.0;
    A[0].Data[1] = 4.0;
    A[0].Data[2] = 7.0;
    A[1].Data[0] = 2.0;
    A[1].Data[1] = 5.0;
    A[1].Data[2] = 8.0;
    A[2].Data[0] = 3.0;
    A[2].Data[1] = 6.0;
    A[2].Data[2] = 9.0;

    /** B = identity → A*B = A */
    B[0].Data[0] = 1.0;
    B[1].Data[1] = 1.0;
    B[2].Data[2] = 1.0;

    const auto R = A * B;
    LUMEN_CHECK_TRUE( R == A );
}

LUMEN_TEST( Matrix_Mul, RvalueOverloads )
{
    const auto A = Maths::FMat4x4f::Translate( Maths::FVec3f( 1.0F, 0.0F, 0.0F ) );
    const auto B = Maths::FMat4x4f::Translate( Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );

    const auto LvalueTimesRvalue = A * Maths::FMat4x4f::Translate( Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );
    const auto RvalueTimesLvalue = Maths::FMat4x4f::Translate( Maths::FVec3f( 1.0F, 0.0F, 0.0F ) ) * B;
    const auto RvalueTimesRvalue = Maths::FMat4x4f::Translate( Maths::FVec3f( 1.0F, 0.0F, 0.0F ) ) *
                                   Maths::FMat4x4f::Translate( Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );

    LUMEN_CHECK_TRUE( Maths::IsEqual( LvalueTimesRvalue, RvalueTimesLvalue ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( RvalueTimesLvalue, RvalueTimesRvalue ) );
}

/**
 *  operator* — matrix × scalar
 */

LUMEN_TEST( Matrix_ScalarMul, FourByFour_Float32_SIMDPath )
{
    const auto M = Maths::FMat4x4f::Identity();
    const auto R = M * 2.0F;

    LUMEN_CHECK_EQ( R[0].Data[0], 2.0F );
    LUMEN_CHECK_EQ( R[1].Data[1], 2.0F );
    LUMEN_CHECK_EQ( R[2].Data[2], 2.0F );
    LUMEN_CHECK_EQ( R[3].Data[3], 2.0F );
    LUMEN_CHECK_EQ( R[0].Data[1], 0.0F );
}

LUMEN_TEST( Matrix_ScalarMul, TwoByTwo_Float64_ScalarPath )
{
    Maths::FMat2x2d M( 0.0 );
    M[0].Data[0] = 1.0;
    M[0].Data[1] = 2.0;
    M[1].Data[0] = 3.0;
    M[1].Data[1] = 4.0;

    const auto R = M * 2.5;

    LUMEN_CHECK_EQ( R[0].Data[0], 2.5 );
    LUMEN_CHECK_EQ( R[0].Data[1], 5.0 );
    LUMEN_CHECK_EQ( R[1].Data[0], 7.5 );
    LUMEN_CHECK_EQ( R[1].Data[1], 10.0 );
}

LUMEN_TEST( Matrix_ScalarMul, ThreeByThree_Float64 )
{
    Maths::FMat3x3d M( 0.0 );
    M[0].Data[0] = 1.0;
    M[1].Data[1] = 1.0;
    M[2].Data[2] = 1.0;

    const auto R = M * 3.0;

    LUMEN_CHECK_EQ( R[0].Data[0], 3.0 );
    LUMEN_CHECK_EQ( R[1].Data[1], 3.0 );
    LUMEN_CHECK_EQ( R[2].Data[2], 3.0 );
}

LUMEN_TEST( Matrix_ScalarMul, NonSquare_Float64 )
{
    Maths::FMat2x3d M( 0.0 );
    for ( USize C = 0; C < 3; ++C )
    {
        for ( USize R = 0; R < 2; ++R )
        {
            M[C].Data[R] = static_cast<Float64>( ( C * 10 ) + R );
        }
    }

    const auto R = M * 2.0;

    for ( USize C = 0; C < 3; ++C )
    {
        for ( USize Row = 0; Row < 2; ++Row )
        {
            LUMEN_CHECK_EQ( R[C].Data[Row], M[C].Data[Row] * 2.0 );
        }
    }
}

LUMEN_TEST( Matrix_ScalarMul, ZeroScalarZeroesMatrix )
{
    const auto M = Maths::FMat4x4f::Identity();
    const auto R = M * 0.0F;

    for ( USize C = 0; C < 4; ++C )
    {
        for ( USize Row = 0; Row < 4; ++Row )
        {
            LUMEN_CHECK_EQ( R[C].Data[Row], 0.0F );
        }
    }
}

/**
 *  TransformAABB — Arvo's method
 */

LUMEN_TEST( TransformAABB, IdentityPreservesBox )
{
    const Maths::FVec3f LocalMin( -1.0F, -2.0F, -3.0F );
    const Maths::FVec3f LocalMax( 1.0F, 2.0F, 3.0F );
    const auto Transform = Maths::FMat4x4f::Identity();

    Maths::FVec3f OutMin;
    Maths::FVec3f OutMax;
    Maths::TransformAABB( LocalMin, LocalMax, Transform, OutMin, OutMax );

    LUMEN_CHECK_TRUE(
        Maths::IsEqual( ( Maths::FMat4x4f::Identity() * 0.0F ) + 0.0F, Maths::FMat4x4f( 0.0F ), 1.0e-6F ) ); /** sanity */
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.X, -1.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Y, -2.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Z, -3.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.X, 1.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Y, 2.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Z, 3.0F ) );
}

LUMEN_TEST( TransformAABB, TranslationShiftsBox )
{
    const Maths::FVec3f LocalMin( -1.0F, -1.0F, -1.0F );
    const Maths::FVec3f LocalMax( 1.0F, 1.0F, 1.0F );
    const auto Transform = Maths::FMat4x4f::Translate( Maths::FVec3f( 10.0F, 20.0F, 30.0F ) );

    Maths::FVec3f OutMin;
    Maths::FVec3f OutMax;
    Maths::TransformAABB( LocalMin, LocalMax, Transform, OutMin, OutMax );

    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.X, 9.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Y, 19.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Z, 29.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.X, 11.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Y, 21.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Z, 31.0F ) );
}

LUMEN_TEST( TransformAABB, UniformScaleScalesBox )
{
    const Maths::FVec3f LocalMin( -1.0F, -2.0F, -3.0F );
    const Maths::FVec3f LocalMax( 1.0F, 2.0F, 3.0F );
    const auto Transform = Maths::FMat4x4f::Scale( Maths::FVec3f( 2.0F, 2.0F, 2.0F ) );

    Maths::FVec3f OutMin;
    Maths::FVec3f OutMax;
    Maths::TransformAABB( LocalMin, LocalMax, Transform, OutMin, OutMax );

    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.X, -2.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Y, -4.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Z, -6.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.X, 2.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Y, 4.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Z, 6.0F ) );
}

LUMEN_TEST( TransformAABB, NonUniformScale )
{
    const Maths::FVec3f LocalMin( -1.0F, -1.0F, -1.0F );
    const Maths::FVec3f LocalMax( 1.0F, 1.0F, 1.0F );
    const auto Transform = Maths::FMat4x4f::Scale( Maths::FVec3f( 2.0F, 3.0F, 4.0F ) );

    Maths::FVec3f OutMin;
    Maths::FVec3f OutMax;
    Maths::TransformAABB( LocalMin, LocalMax, Transform, OutMin, OutMax );

    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.X, -2.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Y, -3.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Z, -4.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.X, 2.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Y, 3.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Z, 4.0F ) );
}

LUMEN_TEST( TransformAABB, QuarterTurnExpandsBox )
{
    /** A cube [-1,1]^3 rotated 45° about Z grows along X and Y. */
    const Float32 Angle  = Maths::Pi<Float32> / 4.0F;
    const auto Transform = Maths::FMat4x4f::RotateZ( Angle );

    Maths::FVec3f OutMin;
    Maths::FVec3f OutMax;
    Maths::TransformAABB( Maths::FVec3f( -1.0F, -1.0F, -1.0F ), Maths::FVec3f( 1.0F, 1.0F, 1.0F ), Transform, OutMin, OutMax );

    /** Half-extent along X/Y is now sqrt(2), ~1.414. */
    const Float32 Sqrt2 = std::numbers::sqrt2_v<Float32>;
    LUMEN_CHECK_TRUE( Maths::IsEqual( std::fabs( OutMin.X ), Sqrt2, 1.0e-4F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( std::fabs( OutMax.X ), Sqrt2, 1.0e-4F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( std::fabs( OutMin.Y ), Sqrt2, 1.0e-4F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( std::fabs( OutMax.Y ), Sqrt2, 1.0e-4F ) );
}

LUMEN_TEST( TransformAABB, BoxMustContainAllEightCorners )
{
    /** Ground truth: transform every corner directly, then min/max. */
    const Maths::FVec3f LocalMin( -1.0F, -2.0F, -3.0F );
    const Maths::FVec3f LocalMax( 1.0F, 2.0F, 3.0F );

    const auto Transform = Maths::FMat4x4f::RotateX( 0.3F ) * Maths::FMat4x4f::RotateY( 0.7F ) *
                           Maths::FMat4x4f::Scale( Maths::FVec3f( 2.0F, 0.5F, 1.5F ) ) *
                           Maths::FMat4x4f::Translate( Maths::FVec3f( 4.0F, -2.0F, 1.0F ) );

    /** Enumerate all 8 corners. */
    const Maths::FVec3f Corners[8] = {
        { LocalMin.X, LocalMin.Y, LocalMin.Z }, { LocalMax.X, LocalMin.Y, LocalMin.Z }, { LocalMin.X, LocalMax.Y, LocalMin.Z },
        { LocalMax.X, LocalMax.Y, LocalMin.Z }, { LocalMin.X, LocalMin.Y, LocalMax.Z }, { LocalMax.X, LocalMin.Y, LocalMax.Z },
        { LocalMin.X, LocalMax.Y, LocalMax.Z }, { LocalMax.X, LocalMax.Y, LocalMax.Z },
    };

    Maths::FVec3f ExpectedMin( std::numeric_limits<Float32>::max() );
    Maths::FVec3f ExpectedMax( -std::numeric_limits<Float32>::max() );

    for ( const auto &Corner : Corners )
    {
        const Maths::FVec3f C = Maths::FVec3f(
            ( Transform[0].X * Corner.X ) + ( Transform[1].X * Corner.Y ) + ( Transform[2].X * Corner.Z ) + Transform[3].X,
            ( Transform[0].Y * Corner.X ) + ( Transform[1].Y * Corner.Y ) + ( Transform[2].Y * Corner.Z ) + Transform[3].Y,
            ( Transform[0].Z * Corner.X ) + ( Transform[1].Z * Corner.Y ) + ( Transform[2].Z * Corner.Z ) + Transform[3].Z );

        ExpectedMin.X = std::min( ExpectedMin.X, C.X );
        ExpectedMin.Y = std::min( ExpectedMin.Y, C.Y );
        ExpectedMin.Z = std::min( ExpectedMin.Z, C.Z );
        ExpectedMax.X = std::max( ExpectedMax.X, C.X );
        ExpectedMax.Y = std::max( ExpectedMax.Y, C.Y );
        ExpectedMax.Z = std::max( ExpectedMax.Z, C.Z );
    }

    Maths::FVec3f OutMin;
    Maths::FVec3f OutMax;
    Maths::TransformAABB( LocalMin, LocalMax, Transform, OutMin, OutMax );

    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.X, ExpectedMin.X, 1.0e-3F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Y, ExpectedMin.Y, 1.0e-3F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Z, ExpectedMin.Z, 1.0e-3F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.X, ExpectedMax.X, 1.0e-3F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Y, ExpectedMax.Y, 1.0e-3F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Z, ExpectedMax.Z, 1.0e-3F ) );
}

LUMEN_TEST( TransformAABB, DegenerateBoxIsPoint )
{
    const Maths::FVec3f P( 1.0F, 2.0F, 3.0F );
    const auto Transform = Maths::FMat4x4f::Translate( Maths::FVec3f( 10.0F, 20.0F, 30.0F ) );

    Maths::FVec3f OutMin;
    Maths::FVec3f OutMax;
    Maths::TransformAABB( P, P, Transform, OutMin, OutMax );

    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.X, 11.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Y, 22.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMin.Z, 33.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.X, 11.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Y, 22.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( OutMax.Z, 33.0F ) );
}

LUMEN_TEST( Matrix_Mul, TwoByTwo_Float32_ScalarPath )
{
    /** Float32 2×2 used to take the SIMD path and corrupt the heap; the dispatch
     *  fix (Rows==4 && Inner==4 && Columns==4) routes it to the scalar fallback. */
    Maths::FMat2x2f A( 0.0F );
    A[0].Data[0] = 1.0F;
    A[0].Data[1] = 2.0F;
    A[1].Data[0] = 3.0F;
    A[1].Data[1] = 4.0F;

    Maths::FMat2x2f B( 0.0F );
    B[0].Data[0] = 5.0F;
    B[0].Data[1] = 6.0F;
    B[1].Data[0] = 7.0F;
    B[1].Data[1] = 8.0F;

    /** Same known values as the Float64 variant. */
    const Maths::FMat2x2f R = A * B;

    LUMEN_CHECK_EQ( R[0].Data[0], 23.0F );
    LUMEN_CHECK_EQ( R[0].Data[1], 34.0F );
    LUMEN_CHECK_EQ( R[1].Data[0], 31.0F );
    LUMEN_CHECK_EQ( R[1].Data[1], 46.0F );
}

LUMEN_TEST( Matrix_Mul, ThreeByThree_Float32_ScalarPath )
{
    Maths::FMat3x3f A( 0.0F );
    A[0].Data[0] = 1.0F;
    A[0].Data[1] = 4.0F;
    A[0].Data[2] = 7.0F;
    A[1].Data[0] = 2.0F;
    A[1].Data[1] = 5.0F;
    A[1].Data[2] = 8.0F;
    A[2].Data[0] = 3.0F;
    A[2].Data[1] = 6.0F;
    A[2].Data[2] = 9.0F;

    Maths::FMat3x3f B = Maths::FMat3x3f::Identity();

    const auto R = A * B;
    LUMEN_CHECK_TRUE( R == A );
}

} // namespace Lumen
