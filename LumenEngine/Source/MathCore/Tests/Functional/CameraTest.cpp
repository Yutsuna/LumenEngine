/**
 * @file CameraTest.cpp
 * @brief Unit tests for FCamera in LumenEngine
 */

#include "Maths/Camera.hpp"
#include <gtest/gtest.h>

namespace
{

/**
 * @class FCameraTest
 * @brief Test fixture for FCamera functional tests
 */
class FCameraTest : public ::testing::Test
{
};

} // namespace

TEST_F( FCameraTest, DefaultConstructor )
{
    LumenEngine::Maths::FCamera Camera;
    EXPECT_EQ( Camera.GetViewMatrix(), LumenEngine::Maths::FMatrix4x4f::Identity() );
    EXPECT_EQ( Camera.GetProjectionMatrix(), LumenEngine::Maths::FMatrix4x4f::Identity() );
}

TEST_F( FCameraTest, SetPerspective )
{
    LumenEngine::Maths::FCamera Camera;
    Camera.SetPerspective( 90.0F, 1.0F, 0.1F, 100.0F );
    Camera.Tick( 0.0 );

    EXPECT_NE( Camera.GetProjectionMatrix(), LumenEngine::Maths::FMatrix4x4f::Identity() );
}

TEST_F( FCameraTest, LookAt )
{
    LumenEngine::Maths::FCamera Camera;
    Camera.LookAt( { 0.0F, 0.0F, 5.0F }, { 0.0F, 0.0F, 0.0F } );
    Camera.Tick( 0.0 );

    EXPECT_NE( Camera.GetViewMatrix(), LumenEngine::Maths::FMatrix4x4f::Identity() );
}

TEST_F( FCameraTest, ViewProjection )
{
    LumenEngine::Maths::FCamera Camera;
    Camera.SetPerspective( 60.0F, 1.77F, 0.1F, 1000.0F );
    Camera.LookAt( { 0.0F, 0.0F, 10.0F }, { 0.0F, 0.0F, 0.0F } );
    Camera.Tick( 0.0 );

    LumenEngine::Maths::FMatrix4x4f ViewProjection = Camera.GetViewProjectionMatrix();
    EXPECT_EQ( ViewProjection, Camera.GetProjectionMatrix() * Camera.GetViewMatrix() );
}
