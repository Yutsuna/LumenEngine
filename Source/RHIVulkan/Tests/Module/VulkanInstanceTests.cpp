/**
 * @file VulkanInstanceTests.cpp
 * @brief Integration tests for Vulkan instance creation.
 */

#include <RHIVulkan/Module/VulkanInstance.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    LUMEN_INTEGRATION_TEST( VulkanInstance, CreateWhenLoaderIsSupported )
    {
        const TExpected<TSharedRef<FVulkanInstance>, RHI::FRHIError> Instance = FVulkanInstance::Create( false );
        if ( not Instance )
        {
            LUMEN_CHECK_EQ( Instance.Error().Code, RHI::ERHIErrorCode::Unsupported );
            return;
        }

        LUMEN_CHECK_TRUE( ( *Instance )->GetHandle() != VK_NULL_HANDLE );
    }

} // namespace RHIVulkan

} // namespace Lumen
