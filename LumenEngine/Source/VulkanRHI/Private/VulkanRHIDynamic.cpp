/**
 * @file VulkanRHIDynamic.hpp
 * @brief Vulkan RHI dynamic resources
 */

#include "VulkanRHIDynamic.hpp"
#include "VulkanRHIViewport.hpp"

#include "Logging/Logger.hpp"
#include "Logging/LoggingCategory.hpp"

namespace
{

static const LumenEngine::FLogCategory LogVulkanRHI( "VulkanRHI" );

}

LumenEngine::RHI::FVulkanRHIDynamic::FVulkanRHIDynamic () : VulkanInstance( VK_NULL_HANDLE )
{
    /* Ctor */
}

LumenEngine::RHI::FVulkanRHIDynamic::~FVulkanRHIDynamic ()
{
    /* Dtor */
}

void LumenEngine::RHI::FVulkanRHIDynamic::Init ()
{
    LUMEN_LOG_INFO( LogVulkanRHI, "Initializing Vulkan RHI dynamic resources..." );
    CreateVulkanInstance();
}

void LumenEngine::RHI::FVulkanRHIDynamic::Shutdown ()
{
    LUMEN_LOG_INFO( LogVulkanRHI, "Shutting down Vulkan RHI dynamic resources..." );
    if ( VulkanInstance != VK_NULL_HANDLE )
    {
        vkDestroyInstance( VulkanInstance, nullptr );
        VulkanInstance = VK_NULL_HANDLE;
    }
}

LumenEngine::TSharedRef<LumenEngine::RHI::IFRHIViewport> LumenEngine::RHI::FVulkanRHIDynamic::RHICreateViewport ( void *InWindowHandle, const Maths::FVec2u &InSize )
{
    return MakeShared<FVulkanRHIViewport>();
}

VkInstance LumenEngine::RHI::FVulkanRHIDynamic::GetVulkanInstance () const
{
    return VulkanInstance;
}

void LumenEngine::RHI::FVulkanRHIDynamic::CreateVulkanInstance ()
{
    VkApplicationInfo AppInfo       = {};
    AppInfo.sType                   = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName        = "Lumen Engine";
    AppInfo.applicationVersion      = VK_MAKE_VERSION( 0, 0, 0 );
    AppInfo.pEngineName             = "Lumen Engine";
    AppInfo.engineVersion           = VK_MAKE_VERSION( 0, 0, 0 );
    AppInfo.apiVersion              = VK_API_VERSION_1_0;
    VkInstanceCreateInfo CreateInfo = {};
    CreateInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.pApplicationInfo     = &AppInfo;

    if ( vkCreateInstance( &CreateInfo, nullptr, &VulkanInstance ) != VK_SUCCESS )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to create Vulkan instance!" );
    }
}
