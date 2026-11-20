/**
 * @file VulkanCheckedConversionTests.cpp
 * @brief Functional tests for overflow-checked RHI-to-Vulkan conversions.
 */

#include <Tests/UnitTest.hpp>
#include <Utilities/VulkanCheckedConversion.hpp>

#include <limits>

namespace Lumen
{

namespace RHIVulkan
{

    LUMEN_TEST( VulkanCheckedConversion, CheckedToVkUInt32AcceptsInRange )
    {
        LUMEN_CHECK_EQ( CheckedToVkUInt32( 0 ).ValueOr( 1U ), 0U );
        LUMEN_CHECK_EQ( CheckedToVkUInt32( std::numeric_limits<UInt32>::max() ).ValueOr( 0U ),
                        std::numeric_limits<UInt32>::max() );
    }

    LUMEN_TEST( VulkanCheckedConversion, CheckedToVkUInt32RejectsNegativeAndOverflow )
    {
        LUMEN_CHECK_HAS_ERROR( CheckedToVkUInt32( -1 ) );
        LUMEN_CHECK_HAS_ERROR( CheckedToVkUInt32( static_cast<Int64>( std::numeric_limits<UInt32>::max() ) + 1 ) );
    }

    LUMEN_TEST( VulkanCheckedConversion, CheckedToVkInt32AcceptsInRange )
    {
        LUMEN_CHECK_EQ( CheckedToVkInt32( std::numeric_limits<Int32>::min() ).ValueOr( 0 ), std::numeric_limits<Int32>::min() );
        LUMEN_CHECK_EQ( CheckedToVkInt32( std::numeric_limits<Int32>::max() ).ValueOr( 0 ), std::numeric_limits<Int32>::max() );
    }

    LUMEN_TEST( VulkanCheckedConversion, CheckedToVkInt32RejectsOutOfRange )
    {
        LUMEN_CHECK_HAS_ERROR( CheckedToVkInt32( static_cast<Int64>( std::numeric_limits<Int32>::max() ) + 1 ) );
        LUMEN_CHECK_HAS_ERROR( CheckedToVkInt32( static_cast<Int64>( std::numeric_limits<Int32>::min() ) - 1 ) );
    }

    LUMEN_TEST( VulkanCheckedConversion, CheckedToVkDeviceSizeAcceptsInRange )
    {
        LUMEN_CHECK_EQ( CheckedToVkDeviceSize( 0ULL ).ValueOr( static_cast<VkDeviceSize>( 1 ) ), static_cast<VkDeviceSize>( 0 ) );
    }

    LUMEN_TEST( VulkanCheckedConversion, CheckedToVkDeviceSizeRejectsNegative )
    {
        LUMEN_CHECK_HAS_ERROR( CheckedToVkDeviceSize( -1 ) );
    }

    LUMEN_TEST( VulkanCheckedConversion, CheckedToUSizeAcceptsInRange )
    {
        LUMEN_CHECK_EQ( CheckedToUSize( 0 ).ValueOr( static_cast<USize>( 1 ) ), static_cast<USize>( 0 ) );
        LUMEN_CHECK_EQ( CheckedToUSize( std::numeric_limits<USize>::max() ).ValueOr( static_cast<USize>( 0 ) ),
                        std::numeric_limits<USize>::max() );
    }

    LUMEN_TEST( VulkanCheckedConversion, CheckedToUSizeRejectsNegative )
    {
        LUMEN_CHECK_HAS_ERROR( CheckedToUSize( -1 ) );
    }

    LUMEN_TEST( VulkanCheckedConversion, CheckedAddUInt64DetectsOverflow )
    {
        LUMEN_CHECK_EQ( CheckedAddUInt64( 4ULL, 5ULL ).ValueOr( 0ULL ), 9ULL );
        LUMEN_CHECK_HAS_ERROR( CheckedAddUInt64( std::numeric_limits<UInt64>::max(), 1ULL ) );
    }

    LUMEN_TEST( VulkanCheckedConversion, CheckedMultiplyUInt64DetectsOverflow )
    {
        LUMEN_CHECK_EQ( CheckedMultiplyUInt64( 6ULL, 7ULL ).ValueOr( 0ULL ), 42ULL );
        LUMEN_CHECK_HAS_ERROR( CheckedMultiplyUInt64( std::numeric_limits<UInt64>::max(), 2ULL ) );
    }

    LUMEN_TEST( VulkanCheckedConversion, ToVulkanViewportAcceptsValidViewport )
    {
        RHI::FRHIViewport Input;
        Input.X        = 0.0F;
        Input.Y        = 0.0F;
        Input.Width    = 128.0F;
        Input.Height   = 64.0F;
        Input.MinDepth = 0.0F;
        Input.MaxDepth = 1.0F;

        const TExpected<VkViewport, RHI::FRHIError> Output = ToVulkanViewport( Input );
        LUMEN_CHECK_TRUE( Output.HasValue() );
        LUMEN_CHECK_EQ( Output->x, 0.0F );
        LUMEN_CHECK_EQ( Output->width, 128.0F );
        LUMEN_CHECK_EQ( Output->height, 64.0F );
    }

    LUMEN_TEST( VulkanCheckedConversion, ToVulkanViewportRejectsNonFinite )
    {
        RHI::FRHIViewport Input;
        Input.Width  = 1.0F;
        Input.Height = 1.0F;
        Input.X      = std::numeric_limits<Float32>::infinity();
        LUMEN_CHECK_HAS_ERROR( ToVulkanViewport( Input ) );
    }

    LUMEN_TEST( VulkanCheckedConversion, ToVulkanViewportRejectsInvalidDimensions )
    {
        RHI::FRHIViewport Input;
        Input.Width  = 0.0F;
        Input.Height = 1.0F;
        LUMEN_CHECK_HAS_ERROR( ToVulkanViewport( Input ) );
    }

} // namespace RHIVulkan

} // namespace Lumen
