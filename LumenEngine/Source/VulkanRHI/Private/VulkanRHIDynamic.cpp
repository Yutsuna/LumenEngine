/**
 * @file VulkanRHIDynamic.hpp
 * @brief Vulkan RHI dynamic resources
 */

#include "VulkanRHIDynamic.hpp"

#include "Logging/Logger.hpp"
#include "Logging/LoggingCategory.hpp"

namespace
{

LumenEngine::FLogCategory LogVulkanRHI( "VulkanRHI" );

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
    if ( VulkanInstance != VK_NULL_HANDLE )
    {
        vkDestroyInstance( VulkanInstance, nullptr );
        VulkanInstance = VK_NULL_HANDLE;
    }
}

LumenEngine::TSharedPtr<LumenEngine::RHI::IFRHIViewport> LumenEngine::RHI::FVulkanRHIDynamic::CreateViewport ( void *InWindowHandle, const Math::FVec2u &InSize )
{
    return MakeShared<FVulkanRHIViewport>();
}

VKInstance LumenEngine::RHI::FVulkanRHIDynamic::GetVulkanInstance () const
{
    return VulkanInstance;
}

void LumenEngine::RHI::FVulkanRHIDynamic::CreateVulkanInstance ()
{
    VkApplicationInfo AppInfo       = {};
    AppInfo.sType                   = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName        = "Lumen Engine";
    AppInfo.applicationVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    AppInfo.pEngineName             = "Lumen Engine";
    AppInfo.engineVersion           = VK_MAKE_VERSION( 1, 0, 0 );
    AppInfo.apiVersion              = VK_API_VERSION_1_0;
    VkInstanceCreateInfo CreateInfo = {};
    CreateInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.pApplicationInfo     = &AppInfo;
    if ( vkCreateInstance( &CreateInfo, nullptr, &VulkanInstance ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create Vulkan instance!" );
    }
}
