/**
 * @file MatrixTest.cpp
 * @brief Unit tests for TMatrix in LumenEngine
 */

#include "Maths/Matrix.hpp"
#include <gtest/gtest.h>

namespace
{

/**
 * @class FMatrixTest
 * @brief Test fixture for TMatrix functional tests
 */
class FMatrixTest : public ::testing::Test
{
protected:

    static constexpr LumenEngine::Float32 Epsilon = 1e-6F;
};

} // namespace

TEST_F( FMatrixTest, DefaultConstructor )
{
    LumenEngine::Maths::FMatrix4x4f Mat;
    for ( LumenEngine::USize ColumnIndex = 0; ColumnIndex < 4; ++ColumnIndex )
    {
        for ( LumenEngine::USize RowIndex = 0; RowIndex < 4; ++RowIndex )
        {
            EXPECT_EQ( Mat[ColumnIndex].Data[RowIndex], 0.0F );
        }
    }
}

TEST_F( FMatrixTest, Identity )
{
    LumenEngine::Maths::FMatrix4x4f Mat = LumenEngine::Maths::FMatrix4x4f::Identity();
    for ( LumenEngine::USize ColumnIndex = 0; ColumnIndex < 4; ++ColumnIndex )
    {
        for ( LumenEngine::USize RowIndex = 0; RowIndex < 4; ++RowIndex )
        {
            if ( ColumnIndex == RowIndex )
            {
                EXPECT_EQ( Mat[ColumnIndex].Data[RowIndex], 1.0F );
            }
            else
            {
                EXPECT_EQ( Mat[ColumnIndex].Data[RowIndex], 0.0F );
            }
        }
    }
}

TEST_F( FMatrixTest, ScalarConstructor )
{
    LumenEngine::Maths::FMatrix4x4f Mat( 2.0F );
    for ( LumenEngine::USize ColumnIndex = 0; ColumnIndex < 4; ++ColumnIndex )
    {
        for ( LumenEngine::USize RowIndex = 0; RowIndex < 4; ++RowIndex )
        {
            if ( ColumnIndex == RowIndex )
            {
                EXPECT_EQ( Mat[ColumnIndex].Data[RowIndex], 2.0F );
            }
            else
            {
                EXPECT_EQ( Mat[ColumnIndex].Data[RowIndex], 0.0F );
            }
        }
    }
}

TEST_F( FMatrixTest, Multiplication )
{
    LumenEngine::Maths::FMatrix4x4f Mat1   = LumenEngine::Maths::FMatrix4x4f::Identity() * 2.0F;
    LumenEngine::Maths::FMatrix4x4f Mat2   = LumenEngine::Maths::FMatrix4x4f::Identity() * 3.0F;
    LumenEngine::Maths::FMatrix4x4f Result = Mat1 * Mat2;

    for ( LumenEngine::USize ColumnIndex = 0; ColumnIndex < 4; ++ColumnIndex )
    {
        for ( LumenEngine::USize RowIndex = 0; RowIndex < 4; ++RowIndex )
        {
            if ( ColumnIndex == RowIndex )
            {
                EXPECT_EQ( Result[ColumnIndex].Data[RowIndex], 6.0F );
            }
            else
            {
                EXPECT_EQ( Result[ColumnIndex].Data[RowIndex], 0.0F );
            }
        }
    }
}

TEST_F( FMatrixTest, Translate )
{
    LumenEngine::Maths::FVec3f Translation( 1.0F, 2.0F, 3.0F );
    LumenEngine::Maths::FMatrix4x4f Mat = LumenEngine::Maths::FMatrix4x4f::Translate( Translation );

    EXPECT_EQ( Mat[3].X, 1.0F );
    EXPECT_EQ( Mat[3].Y, 2.0F );
    EXPECT_EQ( Mat[3].Z, 3.0F );
    EXPECT_EQ( Mat[3].W, 1.0F );
}

TEST_F( FMatrixTest, Scale )
{
    LumenEngine::Maths::FVec3f Scale( 2.0F, 3.0F, 4.0F );
    LumenEngine::Maths::FMatrix4x4f Mat = LumenEngine::Maths::FMatrix4x4f::Scale( Scale );

    EXPECT_EQ( Mat[0].X, 2.0F );
    EXPECT_EQ( Mat[1].Y, 3.0F );
    EXPECT_EQ( Mat[2].Z, 4.0F );
    EXPECT_EQ( Mat[3].W, 1.0F );
}
