/**
 * @file VertexTests.cpp
 * @brief Exhaustive functional tests for Maths::FVertex.
 */

#include "Maths/Macros.hpp"
#include "Maths/Matrix.hpp"
#include <Maths/Vec.hpp>
#include <Maths/Vertex.hpp>
#include <Tests/UnitTest.hpp>

#include <type_traits>

namespace Lumen
{

/**
 *  Compile-time invariants
 */

/** Well-known layout: 3 + 3 + 2 + 3 = 11 floats. */
static_assert( sizeof( Maths::FVertex ) == sizeof( Float32 ) * 11 );
static_assert( alignof( Maths::FVertex ) == alignof( Float32 ) );

/** Public members. */
static_assert( std::is_same_v<decltype( Maths::FVertex{}.Position ), Maths::FVec3f> );
static_assert( std::is_same_v<decltype( Maths::FVertex{}.Normal ), Maths::FVec3f> );
static_assert( std::is_same_v<decltype( Maths::FVertex{}.UV ), Maths::FVec2f> );
static_assert( std::is_same_v<decltype( Maths::FVertex{}.Tangent ), Maths::FVec3f> );

/** Trivially copyable (a vertex buffer needs that). */
static_assert( std::is_trivially_copyable_v<Maths::FVertex> );

/** Default-constructible and equality-comparable at compile time. */
static_assert( std::is_default_constructible_v<Maths::FVertex> );

/** The constructors are constexpr. */
static_assert( []
{
    const Maths::FVertex V( Maths::FVec3f( 1.0F, 2.0F, 3.0F ) );
    return V.Position.Data[0] == 1.0F && V.Position.Data[1] == 2.0F && V.Position.Data[2] == 3.0F;
}() );

/**
 *  Construction
 */

LUMEN_TEST( Vertex_Ctor, DefaultZeroesEverything )
{
    const Maths::FVertex V;

    /** FVec3f / FVec2f default to zeroed, and FVertex doesn't override that. */
    LUMEN_CHECK_TRUE( V.Position == Maths::FVec3f( 0.0F ) );
    LUMEN_CHECK_TRUE( V.Normal == Maths::FVec3f( 0.0F ) );
    LUMEN_CHECK_TRUE( V.UV == Maths::FVec2f( 0.0F ) );
    LUMEN_CHECK_TRUE( V.Tangent == Maths::FVec3f( 0.0F ) );
}

LUMEN_TEST( Vertex_Ctor, PositionOnly )
{
    const Maths::FVertex V( Maths::FVec3f( 1.0F, 2.0F, 3.0F ) );

    LUMEN_CHECK_TRUE( V.Position == Maths::FVec3f( 1.0F, 2.0F, 3.0F ) );
    LUMEN_CHECK_TRUE( V.Normal == Maths::FVec3f( 0.0F ) );
    LUMEN_CHECK_TRUE( V.UV == Maths::FVec2f( 0.0F ) );
    LUMEN_CHECK_TRUE( V.Tangent == Maths::FVec3f( 0.0F ) );
}

LUMEN_TEST( Vertex_Ctor, Full )
{
    const Maths::FVertex V( Maths::FVec3f( 1.0F, 2.0F, 3.0F ), Maths::FVec3f( 0.0F, 1.0F, 0.0F ), Maths::FVec2f( 0.5F, 0.75F ),
                            Maths::FVec3f( 1.0F, 0.0F, 0.0F ) );

    LUMEN_CHECK_TRUE( V.Position == Maths::FVec3f( 1.0F, 2.0F, 3.0F ) );
    LUMEN_CHECK_TRUE( V.Normal == Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );
    LUMEN_CHECK_TRUE( V.UV == Maths::FVec2f( 0.5F, 0.75F ) );
    LUMEN_CHECK_TRUE( V.Tangent == Maths::FVec3f( 1.0F, 0.0F, 0.0F ) );
}

LUMEN_TEST( Vertex_Ctor, Copy )
{
    const Maths::FVertex Original( Maths::FVec3f( 1.0F, 2.0F, 3.0F ), Maths::FVec3f( 0.0F, 1.0F, 0.0F ),
                                   Maths::FVec2f( 0.25F, 0.5F ), Maths::FVec3f( 0.0F, 0.0F, 1.0F ) );

    const Maths::FVertex Copy( Original );

    LUMEN_CHECK_TRUE( Copy == Original );
    LUMEN_CHECK_TRUE( Copy.Position == Original.Position );
    LUMEN_CHECK_TRUE( Copy.Normal == Original.Normal );
    LUMEN_CHECK_TRUE( Copy.UV == Original.UV );
    LUMEN_CHECK_TRUE( Copy.Tangent == Original.Tangent );
}

LUMEN_TEST( Vertex_Ctor, Move )
{
    Maths::FVertex Source( Maths::FVec3f( 4.0F, 5.0F, 6.0F ), Maths::FVec3f( 0.0F, 0.0F, 1.0F ), Maths::FVec2f( 1.0F, 1.0F ),
                           Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );

    const Maths::FVertex Moved( std::move( Source ) );

    LUMEN_CHECK_TRUE( Moved.Position == Maths::FVec3f( 4.0F, 5.0F, 6.0F ) );
    LUMEN_CHECK_TRUE( Moved.Normal == Maths::FVec3f( 0.0F, 0.0F, 1.0F ) );
    LUMEN_CHECK_TRUE( Moved.UV == Maths::FVec2f( 1.0F, 1.0F ) );
    LUMEN_CHECK_TRUE( Moved.Tangent == Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );
}

LUMEN_TEST( Vertex_Ctor, AggregateInitNotAvailable )
{
    /** FVertex has user-declared constructors, so it is not an aggregate. */
    /** This static_assert locks in that fact — a future change that removes */
    /** the constructors would silently change ABI/semantics for consumers. */
    static_assert( !std::is_aggregate_v<Maths::FVertex> );
}

/**
 *  Equality
 */

LUMEN_TEST( Vertex_Equality, IdenticalAreEqual )
{
    const Maths::FVertex A( Maths::FVec3f( 1.0F, 2.0F, 3.0F ), Maths::FVec3f( 0.0F, 1.0F, 0.0F ), Maths::FVec2f( 0.5F, 0.5F ),
                            Maths::FVec3f( 1.0F, 0.0F, 0.0F ) );
    const Maths::FVertex B( Maths::FVec3f( 1.0F, 2.0F, 3.0F ), Maths::FVec3f( 0.0F, 1.0F, 0.0F ), Maths::FVec2f( 0.5F, 0.5F ),
                            Maths::FVec3f( 1.0F, 0.0F, 0.0F ) );

    LUMEN_CHECK_TRUE( A == B );
    LUMEN_CHECK_FALSE( A != B );
}

LUMEN_TEST( Vertex_Equality, DifferentPosition )
{
    Maths::FVertex A;
    Maths::FVertex B;
    B.Position.X = 1.0F;

    LUMEN_CHECK_TRUE( A != B );
    LUMEN_CHECK_FALSE( A == B );
}

LUMEN_TEST( Vertex_Equality, DifferentNormal )
{
    Maths::FVertex A;
    Maths::FVertex B;
    B.Normal.Y = 1.0F;

    LUMEN_CHECK_TRUE( A != B );
}

LUMEN_TEST( Vertex_Equality, DifferentUV )
{
    Maths::FVertex A;
    Maths::FVertex B;
    B.UV.X = 0.5F;

    LUMEN_CHECK_TRUE( A != B );
}

LUMEN_TEST( Vertex_Equality, DifferentTangent )
{
    Maths::FVertex A;
    Maths::FVertex B;
    B.Tangent.Z = 1.0F;

    LUMEN_CHECK_TRUE( A != B );
}

LUMEN_TEST( Vertex_Equality, ReflectedInOperatorNotEqual )
{
    const Maths::FVertex A;
    Maths::FVertex B;
    B.Position.X = 1.0F;

    LUMEN_CHECK_TRUE( A != B );
    LUMEN_CHECK_TRUE( !( A == B ) );
    LUMEN_CHECK_TRUE( !( A != A ) );
}

/**
 *  Member mutation
 */

LUMEN_TEST( Vertex_Members, Mutable )
{
    Maths::FVertex V;
    V.Position = Maths::FVec3f( 1.0F, 2.0F, 3.0F );
    V.Normal   = Maths::FVec3f( 0.0F, 1.0F, 0.0F );
    V.UV       = Maths::FVec2f( 0.1F, 0.2F );
    V.Tangent  = Maths::FVec3f( 1.0F, 0.0F, 0.0F );

    LUMEN_CHECK_TRUE( V.Position == Maths::FVec3f( 1.0F, 2.0F, 3.0F ) );
    LUMEN_CHECK_TRUE( V.Normal == Maths::FVec3f( 0.0F, 1.0F, 0.0F ) );
    LUMEN_CHECK_TRUE( V.UV == Maths::FVec2f( 0.1F, 0.2F ) );
    LUMEN_CHECK_TRUE( V.Tangent == Maths::FVec3f( 1.0F, 0.0F, 0.0F ) );
}

LUMEN_TEST( Vertex_Members, ComponentMutation )
{
    Maths::FVertex V;
    V.Position.X = 1.0F;
    V.Position.Y = 2.0F;
    V.Position.Z = 3.0F;

    LUMEN_CHECK_EQ( V.Position.X, 1.0F );
    LUMEN_CHECK_EQ( V.Position.Y, 2.0F );
    LUMEN_CHECK_EQ( V.Position.Z, 3.0F );
}

/**
 *  Compile-time construction and comparison
 */

LUMEN_TEST( Vertex_Constexpr, FullConstruction )
{
    constexpr Maths::FVertex V( Maths::FVec3f( 1.0F, 2.0F, 3.0F ), Maths::FVec3f( 0.0F, 1.0F, 0.0F ),
                                Maths::FVec2f( 0.5F, 0.25F ), Maths::FVec3f( 1.0F, 0.0F, 0.0F ) );

    static_assert( V.Position.Data[0] == 1.0F );
    static_assert( V.Position.Data[1] == 2.0F );
    static_assert( V.Position.Data[2] == 3.0F );
    static_assert( V.Normal.Data[1] == 1.0F );
    static_assert( V.UV.Data[0] == 0.5F );
    static_assert( V.UV.Data[1] == 0.25F );
    static_assert( V.Tangent.Data[0] == 1.0F );

    LUMEN_CHECK_TRUE( Maths::IsEqual( V.Position.X, 1.0F ) );
}

LUMEN_TEST( Vertex_Constexpr, Comparison )
{
    constexpr Maths::FVertex A;
    constexpr Maths::FVertex B;

    static_assert( A == B );
    static_assert( !( A != B ) );
}

LUMEN_TEST( Vertex_Constexpr, PositionOnly )
{
    constexpr Maths::FVertex V( Maths::FVec3f( 7.0F, 8.0F, 9.0F ) );
    static_assert( V.Position.Data[0] == 7.0F );
    static_assert( V.Normal == Maths::FVec3f( 0.0F ) );
    static_assert( V.UV == Maths::FVec2f( 0.0F ) );
    static_assert( V.Tangent == Maths::FVec3f( 0.0F ) );
}

/**
 *  Byte layout
 */

LUMEN_TEST( Vertex_Layout, OffsetofMatchesStructFieldOrder )
{
    /** C++ doesn't allow offsetof on non-standard-layout types, but FVertex */
    /** is standard-layout. Sanity-check the ordering assumption. */
    static_assert( std::is_standard_layout_v<Maths::FVertex> );

    LUMEN_CHECK_EQ( offsetof( Maths::FVertex, Position ), 0U );
    LUMEN_CHECK_EQ( offsetof( Maths::FVertex, Normal ), sizeof( Float32 ) * 3 );
    LUMEN_CHECK_EQ( offsetof( Maths::FVertex, UV ), sizeof( Float32 ) * 6 );
    LUMEN_CHECK_EQ( offsetof( Maths::FVertex, Tangent ), sizeof( Float32 ) * 8 );
}

LUMEN_TEST( Vertex_Layout, RawByteEqualityImpliesVertexEquality )
{
    /** Two vertices with identical memory contents are equal — this is the */
    /** property graphics pipelines rely on when hashing vertex buffers. */
    Maths::FVertex A;
    A.Position = Maths::FVec3f( 1.0F, 2.0F, 3.0F );
    A.Normal   = Maths::FVec3f( 0.0F, 1.0F, 0.0F );
    A.UV       = Maths::FVec2f( 0.1F, 0.2F );
    A.Tangent  = Maths::FVec3f( 1.0F, 0.0F, 0.0F );

    Maths::FVertex B;
    B.Position = A.Position;
    B.Normal   = A.Normal;
    B.UV       = A.UV;
    B.Tangent  = A.Tangent;

    LUMEN_CHECK_TRUE( A == B );
}

/**
 *  Interoperability with the rest of the module
 */

LUMEN_TEST( Vertex_Interop, PositionCanBeTransformedByMatrix )
{
    Maths::FVertex V;
    V.Position = Maths::FVec3f( 1.0F, 0.0F, 0.0F );

    const auto Translate = Maths::FMat4x4f::Translate( Maths::FVec3f( 10.0F, 0.0F, 0.0F ) );

    /** Manual w=1 homogeneous multiply (mirrors what a real renderer does). */
    const auto &M         = Translate;
    const Maths::FVec3f P = V.Position;
    const Maths::FVec3f Transformed( ( M[0].X * P.X ) + ( M[1].X * P.Y ) + ( M[2].X * P.Z ) + M[3].X,
                                     ( M[0].Y * P.X ) + ( M[1].Y * P.Y ) + ( M[2].Y * P.Z ) + M[3].Y,
                                     ( M[0].Z * P.X ) + ( M[1].Z * P.Y ) + ( M[2].Z * P.Z ) + M[3].Z );

    LUMEN_CHECK_TRUE( Maths::IsEqual( Transformed.X, 11.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( Transformed.Y, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( Transformed.Z, 0.0F ) );
}

LUMEN_TEST( Vertex_Interop, UVVectorArithmetic )
{
    /** Common use: sample-space translation. */
    Maths::FVertex V;
    V.UV = Maths::FVec2f( 0.25F, 0.5F );
    V.UV = V.UV * 2.0F;

    LUMEN_CHECK_TRUE( Maths::IsEqual( V.UV.X, 0.5F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( V.UV.Y, 1.0F ) );
}

LUMEN_TEST( Vertex_Interop, NormalizeNormal )
{
    Maths::FVertex V;
    V.Normal = Maths::FVec3f( 0.0F, 2.0F, 0.0F );
    V.Normal = V.Normal.Normalize();

    LUMEN_CHECK_TRUE( Maths::IsEqual( V.Normal.X, 0.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( V.Normal.Y, 1.0F ) );
    LUMEN_CHECK_TRUE( Maths::IsEqual( V.Normal.Z, 0.0F ) );
}

} // namespace Lumen
