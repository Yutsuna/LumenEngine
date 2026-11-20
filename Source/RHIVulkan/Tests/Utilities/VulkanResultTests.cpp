/**
 * @file VulkanResultTests.cpp
 * @brief Functional tests for Vulkan result code translation.
 */

#include <Tests/UnitTest.hpp>
#include <Utilities/VulkanResult.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    LUMEN_TEST( VulkanResult, SuccessMapsToBackendFailure )
    {
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_SUCCESS, false ), RHI::ERHIErrorCode::BackendFailure );
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_SUCCESS, true ), RHI::ERHIErrorCode::BackendFailure );
    }

    LUMEN_TEST( VulkanResult, NotReadyMapsToInvalidStateOrTimeout )
    {
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_NOT_READY, false ), RHI::ERHIErrorCode::InvalidState );
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_NOT_READY, true ), RHI::ERHIErrorCode::Timeout );
    }

    LUMEN_TEST( VulkanResult, TimeoutMapsToTimeout )
    {
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_TIMEOUT, false ), RHI::ERHIErrorCode::Timeout );
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_TIMEOUT, true ), RHI::ERHIErrorCode::Timeout );
    }

    LUMEN_TEST( VulkanResult, OutOfMemoryMapsToOutOfMemory )
    {
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_ERROR_OUT_OF_HOST_MEMORY, false ), RHI::ERHIErrorCode::OutOfMemory );
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_ERROR_OUT_OF_DEVICE_MEMORY, false ), RHI::ERHIErrorCode::OutOfMemory );
    }

    LUMEN_TEST( VulkanResult, DeviceLostMapsToDeviceLost )
    {
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_ERROR_DEVICE_LOST, false ), RHI::ERHIErrorCode::DeviceLost );
    }

    LUMEN_TEST( VulkanResult, UnsupportedErrorsMapToUnsupported )
    {
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_ERROR_INCOMPATIBLE_DRIVER, false ), RHI::ERHIErrorCode::Unsupported );
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_ERROR_FEATURE_NOT_PRESENT, false ), RHI::ERHIErrorCode::Unsupported );
    }

    LUMEN_TEST( VulkanResult, PresentationErrorsMapToOutOfDate )
    {
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_ERROR_SURFACE_LOST_KHR, false ), RHI::ERHIErrorCode::OutOfDate );
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( VK_ERROR_OUT_OF_DATE_KHR, false ), RHI::ERHIErrorCode::OutOfDate );
    }

    LUMEN_TEST( VulkanResult, UnknownResultMapsToBackendFailure )
    {
        LUMEN_CHECK_EQ( TranslateVulkanResultCode( static_cast<VkResult>( -999 ), false ), RHI::ERHIErrorCode::BackendFailure );
    }

} // namespace RHIVulkan

} // namespace Lumen
