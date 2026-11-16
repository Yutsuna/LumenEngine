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
    LUMEN_CHECK_TRUE( ToRHIString( ERHIPresentMode::Fifo ) == "ERHIPresentMode::Fifo" );
    LUMEN_CHECK_TRUE( ToRHIString( ERHIColorSpace::SrgbNonLinear ) == "ERHIColorSpace::SrgbNonLinear" );
    LUMEN_CHECK_TRUE( ToRHIString( ERHISurfaceType::Null ) == "ERHISurfaceType::Null" );
    LUMEN_CHECK_TRUE( ToRHIString( ERHIQueryType::Timestamp ) == "ERHIQueryType::Timestamp" );
    LUMEN_CHECK_TRUE( ToRHIString( ERHIDebugSeverity::Warning ) == "ERHIDebugSeverity::Warning" );
    LUMEN_CHECK_TRUE( ToRHIString( ERHIDebugCategory::Validation ) == "ERHIDebugCategory::Validation" );
    LUMEN_CHECK_TRUE( ToRHIString( ERHICacheEvictionPolicy::None ) == "ERHICacheEvictionPolicy::None" );
    LUMEN_CHECK_TRUE( ToRHIString( ERHIErrorCode::OutOfDate ) == "ERHIErrorCode::OutOfDate" );
    LUMEN_CHECK_TRUE( ToRHIString( static_cast<ERHIFormat>( 0xFFFFU ) ) == "Unknown" );
}

} // namespace Lumen
