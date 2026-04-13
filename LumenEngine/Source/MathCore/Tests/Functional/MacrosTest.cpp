/**
 * @file MacrosTest.cpp
 * @brief Unit tests for mathematical constants and macros in LumenEngine
 */

#include "Maths/Macros.hpp"
#include <gtest/gtest.h>

namespace
{

/**
 * @class FMacrosTest
 * @brief Test fixture for mathematical constants and macros functional tests
 */
class FMacrosTest : public ::testing::Test
{
protected:

    static constexpr LumenEngine::Float64 Precision = 1e-10;
};

} // namespace

TEST_F( FMacrosTest, Constants )
{
    EXPECT_NEAR( LumenEngine::Maths::Pi, 3.14159265358979323846, Precision );
    EXPECT_NEAR( LumenEngine::Maths::TwoPi, LumenEngine::Maths::Pi * 2.0, Precision );
    EXPECT_NEAR( LumenEngine::Maths::HalfPi, LumenEngine::Maths::Pi / 2.0, Precision );
}

TEST_F( FMacrosTest, Conversion )
{
    EXPECT_NEAR( 180.0 * LumenEngine::Maths::DegToRad, LumenEngine::Maths::Pi, Precision );
    EXPECT_NEAR( LumenEngine::Maths::Pi * LumenEngine::Maths::RadToDeg, 180.0, Precision );
}
