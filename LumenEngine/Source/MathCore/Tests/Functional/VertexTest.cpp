/**
 * @file VertexTest.cpp
 * @brief Unit tests for FVertex in LumenEngine
 */

#include "Maths/Vertex.hpp"
#include <gtest/gtest.h>

namespace
{

    /**
 * @class FVertexTest
 * @brief Test fixture for FVertex functional tests
 */
class FVertexTest : public ::testing::Test
{
};

} // namespace

TEST_F( FVertexTest, DefaultConstructor )
{
    LumenEngine::Maths::FVertex Vertex;
    EXPECT_EQ( Vertex.Position.X, 0.0F );
    EXPECT_EQ( Vertex.Normal.X, 0.0F );
    EXPECT_EQ( Vertex.UV.X, 0.0F );
    EXPECT_EQ( Vertex.Tangent.X, 0.0F );
}

TEST_F( FVertexTest, PositionConstructor )
{
    LumenEngine::Maths::FVec3f Position( 1.0F, 2.0F, 3.0F );
    LumenEngine::Maths::FVertex Vertex( Position );

    EXPECT_EQ( Vertex.Position, Position );
    EXPECT_EQ( Vertex.Normal.X, 0.0F );
}

TEST_F( FVertexTest, FullConstructor )
{
    LumenEngine::Maths::FVec3f Position( 1.0F, 2.0F, 3.0F );
    LumenEngine::Maths::FVec3f Normal( 0.0F, 1.0F, 0.0F );
    LumenEngine::Maths::FVec2f UV( 0.5F, 0.5F );
    LumenEngine::Maths::FVec3f Tangent( 1.0F, 0.0F, 0.0F );

    LumenEngine::Maths::FVertex Vertex( Position, Normal, UV, Tangent );

    EXPECT_EQ( Vertex.Position, Position );
    EXPECT_EQ( Vertex.Normal, Normal );
    EXPECT_EQ( Vertex.UV, UV );
    EXPECT_EQ( Vertex.Tangent, Tangent );
}

TEST_F( FVertexTest, Equality )
{
    LumenEngine::Maths::FVertex Vertex1( { 1.0F, 1.0F, 1.0F } );
    LumenEngine::Maths::FVertex Vertex2( { 1.0F, 1.0F, 1.0F } );
    LumenEngine::Maths::FVertex Vertex3( { 2.0F, 2.0F, 2.0F } );

    EXPECT_TRUE( Vertex1 == Vertex2 );
    EXPECT_FALSE( Vertex1 == Vertex3 );
}
