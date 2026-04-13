/**
 * @file VecTest.cpp
 * @brief Unit tests for TVec in LumenEngine
 */

#include "Maths/Vec.hpp"
#include <gtest/gtest.h>

namespace
{

/**
 * @class FVecTest
 * @brief Test fixture for TVec functional tests
 */
class FVecTest : public ::testing::Test
{
protected:

    static constexpr LumenEngine::Float32 Epsilon = 1e-6F;
};

}

TEST_F( FVecTest, DefaultConstructor )
{
    const LumenEngine::Maths::FVec3f Vec;
    EXPECT_EQ( Vec.X, 0.0F );
    EXPECT_EQ( Vec.Y, 0.0F );
    EXPECT_EQ( Vec.Z, 0.0F );
}

TEST_F( FVecTest, ScalarConstructor )
{
    const LumenEngine::Maths::FVec3f Vec( 1.0F );
    EXPECT_EQ( Vec.X, 1.0F );
    EXPECT_EQ( Vec.Y, 1.0F );
    EXPECT_EQ( Vec.Z, 1.0F );
}

TEST_F( FVecTest, ComponentConstructor )
{
    const LumenEngine::Maths::FVec2f Vec2( 1.0F, 2.0F );
    EXPECT_EQ( Vec2.X, 1.0F );
    EXPECT_EQ( Vec2.Y, 2.0F );

    const LumenEngine::Maths::FVec3f Vec3( 1.0F, 2.0F, 3.0F );
    EXPECT_EQ( Vec3.X, 1.0F );
    EXPECT_EQ( Vec3.Y, 2.0F );
    EXPECT_EQ( Vec3.Z, 3.0F );

    const LumenEngine::Maths::FVec4f Vec4( 1.0F, 2.0F, 3.0F, 4.0F );
    EXPECT_EQ( Vec4.X, 1.0F );
    EXPECT_EQ( Vec4.Y, 2.0F );
    EXPECT_EQ( Vec4.Z, 3.0F );
    EXPECT_EQ( Vec4.W, 4.0F );
}

TEST_F( FVecTest, Equality )
{
    const LumenEngine::Maths::FVec3f Vec1( 1.0F, 2.0F, 3.0F );
    const LumenEngine::Maths::FVec3f Vec2( 1.0F, 2.0F, 3.0F );
    const LumenEngine::Maths::FVec3f Vec3( 3.0F, 2.0F, 1.0F );

    EXPECT_TRUE( Vec1 == Vec2 );
    EXPECT_FALSE( Vec1 == Vec3 );
    EXPECT_FALSE( Vec1 != Vec2 );
    EXPECT_TRUE( Vec1 != Vec3 );
}

TEST_F( FVecTest, Addition )
{
    const LumenEngine::Maths::FVec3f Vec1( 1.0F, 2.0F, 3.0F );
    const LumenEngine::Maths::FVec3f Vec2( 4.0F, 5.0F, 6.0F );
    const LumenEngine::Maths::FVec3f Result = Vec1 + Vec2;

    EXPECT_EQ( Result.X, 5.0F );
    EXPECT_EQ( Result.Y, 7.0F );
    EXPECT_EQ( Result.Z, 9.0F );

    const LumenEngine::Maths::FVec3f ScalarResult = Vec1 + 10.0F;
    EXPECT_EQ( ScalarResult.X, 11.0F );
    EXPECT_EQ( ScalarResult.Y, 12.0F );
    EXPECT_EQ( ScalarResult.Z, 13.0F );
}

TEST_F( FVecTest, Subtraction )
{
    const LumenEngine::Maths::FVec3f Vec1( 10.0F, 10.0F, 10.0F );
    const LumenEngine::Maths::FVec3f Vec2( 1.0F, 2.0F, 3.0F );
    const LumenEngine::Maths::FVec3f Result = Vec1 - Vec2;

    EXPECT_EQ( Result.X, 9.0F );
    EXPECT_EQ( Result.Y, 8.0F );
    EXPECT_EQ( Result.Z, 7.0F );
}

TEST_F( FVecTest, Multiplication )
{
    const LumenEngine::Maths::FVec3f Vec1( 1.0F, 2.0F, 3.0F );
    const LumenEngine::Maths::FVec3f Vec2( 2.0F, 3.0F, 4.0F );
    const LumenEngine::Maths::FVec3f Result = Vec1 * Vec2;

    EXPECT_EQ( Result.X, 2.0F );
    EXPECT_EQ( Result.Y, 6.0F );
    EXPECT_EQ( Result.Z, 12.0F );

    const LumenEngine::Maths::FVec3f ScalarResult = Vec1 * 2.0F;
    EXPECT_EQ( ScalarResult.X, 2.0F );
    EXPECT_EQ( ScalarResult.Y, 4.0F );
    EXPECT_EQ( ScalarResult.Z, 6.0F );
}

TEST_F( FVecTest, Division )
{
    const LumenEngine::Maths::FVec3f Vec1( 10.0F, 20.0F, 30.0F );
    const LumenEngine::Maths::FVec3f ScalarResult = Vec1 / 2.0F;

    EXPECT_EQ( ScalarResult.X, 5.0F );
    EXPECT_EQ( ScalarResult.Y, 10.0F );
    EXPECT_EQ( ScalarResult.Z, 15.0F );
}

TEST_F( FVecTest, DotProduct )
{
    const LumenEngine::Maths::FVec3f Vec1( 1.0F, 2.0F, 3.0F );
    const LumenEngine::Maths::FVec3f Vec2( 4.0F, 5.0F, 6.0F );
    LumenEngine::Float32 DotValue = Vec1.Dot( Vec2 );

    EXPECT_EQ( DotValue, (1.0F * 4.0F) + (2.0F * 5.0F) + (3.0F * 6.0F) );
}

TEST_F( FVecTest, CrossProduct )
{
    const LumenEngine::Maths::FVec3f Vec1( 1.0F, 0.0F, 0.0F );
    const LumenEngine::Maths::FVec3f Vec2( 0.0F, 1.0F, 0.0F );
    const LumenEngine::Maths::FVec3f CrossValue = Vec1.Cross( Vec2 );

    EXPECT_EQ( CrossValue.X, 0.0F );
    EXPECT_EQ( CrossValue.Y, 0.0F );
    EXPECT_EQ( CrossValue.Z, 1.0F );
}

TEST_F( FVecTest, Length )
{
    const LumenEngine::Maths::FVec3f Vec( 3.0F, 4.0F, 0.0F );
    EXPECT_EQ( Vec.LengthSquared(), 25.0F );
    EXPECT_EQ( Vec.Length(), 5.0F );
}

TEST_F( FVecTest, Normalize )
{
    const LumenEngine::Maths::FVec3f Vec( 10.0F, 0.0F, 0.0F );
    const LumenEngine::Maths::FVec3f Normalized = Vec.Normalize();

    EXPECT_NEAR( Normalized.X, 1.0F, Epsilon );
    EXPECT_NEAR( Normalized.Y, 0.0F, Epsilon );
    EXPECT_NEAR( Normalized.Z, 0.0F, Epsilon );
    EXPECT_NEAR( Normalized.Length(), 1.0F, Epsilon );
}

TEST_F( FVecTest, Negation )
{
    const LumenEngine::Maths::FVec3f Vec( 1.0F, -2.0F, 3.0F );
    const LumenEngine::Maths::FVec3f Negated = -Vec;

    EXPECT_EQ( Negated.X, -1.0F );
    EXPECT_EQ( Negated.Y, 2.0F );
    EXPECT_EQ( Negated.Z, -3.0F );
}

TEST_F( FVecTest, Aliases )
{
    const LumenEngine::Maths::FVec4f Color( 1.0F, 0.5F, 0.2F, 1.0F );
    EXPECT_EQ( Color.R, 1.0F );
    EXPECT_EQ( Color.G, 0.5F );
    EXPECT_EQ( Color.B, 0.2F );
    EXPECT_EQ( Color.A, 1.0F );

    const LumenEngine::Maths::FVec2f TexCoord( 0.1F, 0.9F );
    EXPECT_EQ( TexCoord.S, 0.1F );
    EXPECT_EQ( TexCoord.T, 0.9F );
}
