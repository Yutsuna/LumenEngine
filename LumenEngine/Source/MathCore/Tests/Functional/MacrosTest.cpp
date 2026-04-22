/**
 * @file MacrosTest.cpp
 * @brief Unit tests for mathematical constants and macros in LumenEngine
 */

#include "Maths/Macros.hpp"
#include "CoreTypes.hpp"

#include <gtest/gtest.h>

namespace LumenEngine
{

namespace
{

    /**
     * @class FMacrosTest
     * @brief Test fixture for mathematical constants and macros functional tests
     */
    class FMacrosTest : public ::testing::Test
    {
    protected:

        static constexpr Float64 Precision = 1e-10;
    };

} // namespace

TEST_F( FMacrosTest, Constants )
{
    EXPECT_NEAR( Maths::Pi<Float64>, 3.14159265358979323846, Precision );
    EXPECT_NEAR( Maths::TwoPi<Float64>, Maths::Pi<Float64> * 2.0, Precision );
    EXPECT_NEAR( Maths::HalfPi<Float64>, Maths::Pi<Float64> / 2.0, Precision );
}

TEST_F( FMacrosTest, Conversion )
{
    EXPECT_NEAR( 180.0 * Maths::DegToRad<Float64>, Maths::Pi<Float64>, Precision );
    EXPECT_NEAR( Maths::Pi<Float64> * Maths::RadToDeg<Float64>, 180.0, Precision );
}

} // namespace LumenEngine