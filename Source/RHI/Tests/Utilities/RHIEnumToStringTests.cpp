/**
 * @file RHIEnumToStringTests.cpp
 * @brief Tests stable names and unknown fallbacks.
 */

#include <Tests/UnitTest.hpp>
#include <Utilities/RHIEnumToString.hpp>

namespace Lumen
{

LUMEN_TEST( RHIEnumToString, CoversRepresentativeEnums )
{
    LUMEN_CHECK_TRUE( ToRHIString( ERHIFormat::RGBA8Unorm ) == "ERHIFormat::RGBA8Unorm" );
    LUMEN_CHECK_TRUE( ToRHIString( ERHIErrorCode::DeviceLost ) == "ERHIErrorCode::DeviceLost" );
    LUMEN_CHECK_TRUE( ToRHIString( ERHIQueueType::Graphics ) == "ERHIQueueType::Graphics" );
    LUMEN_CHECK_TRUE( ToRHIString( ERHIShaderStage::Compute ) == "ERHIShaderStage::Compute" );
    LUMEN_CHECK_TRUE( ToRHIString( static_cast<ERHIFormat>( 0xFFFFU ) ) == "Unknown" );
}

} // namespace Lumen
