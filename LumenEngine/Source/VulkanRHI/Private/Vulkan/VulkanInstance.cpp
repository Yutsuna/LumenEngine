/**
 * @file VulkanInstance.cpp
 * @brief Implementation of the FVulkanInstance class.
 */

#include "Vulkan/VulkanInstance.hpp"
#include "Generic/GenericWindow.hpp"
#include "Vulkan/VulkanCore.hpp"

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    #include "Linux/LinuxWindow.hpp"
    #include <SDL3/SDL_vulkan.h>
#endif

namespace
{

inline VkApplicationInfo GetAppInfo ()
{
    return { .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
             .pNext              = VK_NULL_HANDLE,
             .pApplicationName   = "Lumen Engine",
             .applicationVersion = VK_MAKE_API_VERSION( 0, 0, 1, 0 ),
             .pEngineName        = "Lumen Engine",
             .engineVersion      = VK_MAKE_API_VERSION( 0, 0, 1, 0 ),
             .apiVersion         = VK_API_VERSION_1_3

    };
}

inline VkInstanceCreateInfo GetInstanceCreateInfo ( const VkApplicationInfo &AppInfo )
{
    return { .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
             .pNext                   = VK_NULL_HANDLE,
             .flags                   = 0,
             .pApplicationInfo        = &AppInfo,
             .enabledLayerCount       = 0,
             .ppEnabledLayerNames     = nullptr,
             .enabledExtensionCount   = 0,
             .ppEnabledExtensionNames = nullptr };
}

} // namespace

void LumenEngine::VulkanRHI::FVulkanInstance::Initialize ( const TSharedPtr<FGenericWindow> &InWindow )
{
    const VkApplicationInfo AppInfo = GetAppInfo();
    VkInstanceCreateInfo CreateInfo = GetInstanceCreateInfo( AppInfo );

    CreateInstance( CreateInfo, InWindow );
}

void LumenEngine::VulkanRHI::FVulkanInstance::Cleanup () noexcept
{
    if ( Surface != VK_NULL_HANDLE )
    {
        vkDestroySurfaceKHR( Instance, Surface, nullptr );
        Surface = VK_NULL_HANDLE;
    }

    if ( Instance != VK_NULL_HANDLE )
    {
        vkDestroyInstance( Instance, nullptr );
        Instance = VK_NULL_HANDLE;
    }
}

VkInstance LumenEngine::VulkanRHI::FVulkanInstance::GetHandle () const noexcept
{
    return Instance;
}

VkSurfaceKHR LumenEngine::VulkanRHI::FVulkanInstance::GetSurface () const noexcept
{
    return Surface;
}

void LumenEngine::VulkanRHI::FVulkanInstance::CreateInstance ( VkInstanceCreateInfo &CreateInfo, const TSharedPtr<FGenericWindow> &InWindow )
{
#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    UInt32 ExtensionCount                = 0;
    const AnsiChar *const *SDLExtensions = SDL_Vulkan_GetInstanceExtensions( &ExtensionCount );
    CreateInfo.enabledExtensionCount     = ExtensionCount;
    CreateInfo.ppEnabledExtensionNames   = SDLExtensions;
#endif

#if !defined( NDEBUG )
    const AnsiChar *ValidationLayers[] = { "VK_LAYER_KHRONOS_validation" };
    CreateInfo.enabledLayerCount       = 1;
    CreateInfo.ppEnabledLayerNames     = ValidationLayers;
    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan Validation Layers ENABLED." );
#else
#endif

    LUMEN_VK_CHECK( vkCreateInstance( &CreateInfo, nullptr, &Instance ) );
    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan Instance created successfully." );

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    TSharedPtr<FLinuxWindow> LinuxWindow = StaticCastSharedPtr<FLinuxWindow>( InWindow );
    SDL_Window *OSWindow                 = LinuxWindow->GetOSWindowHandle();

    if ( not SDL_Vulkan_CreateSurface( OSWindow, Instance, nullptr, &Surface ) )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to create Vulkan Surface: {}", SDL_GetError() );
    }
#else
    LUMEN_LOG_FATAL( LogVulkanRHI, "Unsupported platform for surface creation." );
#endif
}
