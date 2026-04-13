/**
 * @file SIMDTest.cpp
 * @brief Functional tests for SIMD utilities in LumenEngine
 */

#include "Maths/SIMD/SIMD.hpp"
#include <gtest/gtest.h>

namespace
{

/**
 * @class FSIMDTest
 * @brief Test fixture for SIMD functional tests
 */
class FSIMDTest : public ::testing::Test
{
protected:

    static constexpr LumenEngine::Float32 Epsilon = 1e-6F;
};

} // namespace

TEST_F( FSIMDTest, Alignment )
{
    /** Verify that the SIMD register type has the required 16-byte alignment. */
    EXPECT_EQ( alignof( LumenEngine::Maths::SIMD::FSimdFloat ), 16 );
}

TEST_F( FSIMDTest, LoadAndStore )
{
    using namespace LumenEngine::Maths::SIMD;

    /** Ensure input data is 16-byte aligned for SIMD operations. */
    alignas( 16 ) const LumenEngine::Float32 InputData[4] = { 1.0F, 2.0F, 3.0F, 4.0F };
    alignas( 16 ) LumenEngine::Float32 OutputData[4] = { 0.0F, 0.0F, 0.0F, 0.0F };

    const FSimdFloat Register = FSimdFloat::Load( InputData );
    Register.Store( OutputData );

    for ( LumenEngine::USize i = 0; i < 4; ++i )
    {
        EXPECT_EQ( InputData[i], OutputData[i] );
    }
}

TEST_F( FSIMDTest, MatrixMul4x4 )
{
    using namespace LumenEngine::Maths::SIMD;

    /**
     * Matrix multiplication test.
     * Matrices are stored in column-major order or row-major order?
     * Looking at SIMDSSE.inl implementation:
     * L0, L1, L2, L3 are loaded from InLeft + 0, 4, 8, 12.
     * RCol is loaded from InRight + (Idx * 4).
     * This suggests column-major layout where each L_i is a column and each RCol is a column.
     */

    /** Identity Matrix */
    alignas( 16 ) const LumenEngine::Float32 MatrixA[16] = {
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    };

    /** Arbitrary Matrix */
    alignas( 16 ) const LumenEngine::Float32 MatrixB[16] = {
        1.0F, 5.0F, 9.0F, 13.0F,
        2.0F, 6.0F, 10.0F, 14.0F,
        3.0F, 7.0F, 11.0F, 15.0F,
        4.0F, 8.0F, 12.0F, 16.0F
    };

    alignas( 16 ) LumenEngine::Float32 Result[16] = { 0.0F };

    /** Identity * B = B */
    MatrixMul4x4( MatrixA, MatrixB, Result );

    for ( LumenEngine::USize i = 0; i < 16; ++i )
    {
        EXPECT_NEAR( Result[i], MatrixB[i], Epsilon );
    }

    /** B * Identity = B */
    MatrixMul4x4( MatrixB, MatrixA, Result );

    for ( LumenEngine::USize i = 0; i < 16; ++i )
    {
        EXPECT_NEAR( Result[i], MatrixB[i], Epsilon );
    }

    /** Specific multiplication: Diagonal * Diagonal */
    alignas( 16 ) const LumenEngine::Float32 MatrixD1[16] = {
        2.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 2.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 2.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 2.0F
    };

    alignas( 16 ) const LumenEngine::Float32 MatrixD2[16] = {
        3.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 3.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 3.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 3.0F
    };

    MatrixMul4x4( MatrixD1, MatrixD2, Result );

    for ( LumenEngine::USize i = 0; i < 16; ++i )
    {
        if ( i == 0 || i == 5 || i == 10 || i == 15 )
        {
            EXPECT_NEAR( Result[i], 6.0F, Epsilon );
        }
        else
        {
            EXPECT_NEAR( Result[i], 0.0F, Epsilon );
        }
    }
}
