/**
 * @file VulkanInstanceTests.cpp
 * @brief Integration tests for Vulkan instance creation.
 */

#include <RHIVulkan/Module/VulkanApplication.hpp>
#include <RHIVulkan/Module/VulkanInstance.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    LUMEN_TEST( VulkanApplication, RequestsVulkan14Instance )
    {
        const FVulkanApplication Application;
        const VkApplicationInfo ApplicationInfo = Application.MakeApplicationInfo();
        LUMEN_CHECK_EQ( ApplicationInfo.apiVersion, VK_API_VERSION_1_4 );
    }

    LUMEN_INTEGRATION_TEST( VulkanInstance, CreateWhenLoaderIsSupported )
    {
        const TExpected<TSharedRef<FVulkanInstance>, RHI::FRHIError> Instance = FVulkanInstance::Create( false );
        LUMEN_CHECK_TRUE( Instance.HasValue() );

        LUMEN_CHECK_TRUE( ( *Instance )->GetHandle() != VK_NULL_HANDLE );
    }

} // namespace RHIVulkan

} // namespace Lumen
