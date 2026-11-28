/**
 * @file VulkanApplication.cpp
 * @brief Implements Vulkan instance creation parameter selection.
 */

#include <RHIVulkan/Module/VulkanApplication.hpp>
#include <RHIVulkan/Module/VulkanExtensions.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <Algo/ForEach.hpp>

/**
 * Private
 */

namespace
{

VKAPI_ATTR VkBool32 VKAPI_CALL BootstrapDebugCallback ( VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                        VkDebugUtilsMessageTypeFlagsEXT,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *,
                                                        void * ) noexcept
{
    return VK_FALSE;
}

[[nodiscard]] Lumen::RHI::FRHIExpected ValidateLoaderVersion () noexcept
{
    const auto EnumerateVersion =
        reinterpret_cast<PFN_vkEnumerateInstanceVersion>( vkGetInstanceProcAddr( nullptr, "vkEnumerateInstanceVersion" ) );
    if ( EnumerateVersion == nullptr )
    {
        return Lumen::MakeUnexpected( Lumen::RHI::MakeRHIUnsupported( Lumen::RHI::ERHIErrorCondition::Capability,
                                                                      "Vulkan loader does not expose Vulkan 1.4." ) );
    }

    Lumen::UInt32 LoaderVersion = 0U;
    LUMEN_VK_TRY( EnumerateVersion( &LoaderVersion ), Lumen::RHI::ERHIErrorCondition::Device );

    if ( LoaderVersion < VK_API_VERSION_1_4 )
    {
        return Lumen::MakeUnexpected( Lumen::RHI::MakeRHIUnsupported(
            Lumen::RHI::ERHIErrorCondition::Capability, "Vulkan loader API version is below the required 1.4 baseline." ) );
    }

    return {};
}

} // namespace

/**
 * Static
 */

Lumen::TExpected<Lumen::RHIVulkan::FVulkanApplication, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanApplication::Create ( const Bool bEnableDebug ) noexcept
{
    LUMEN_EXPECT( LoaderValidation, ValidateLoaderVersion() );
    LUMEN_EXPECT( Available, FVulkanExtensions::Enumerate() );

    FVulkanApplication Application;

    const auto EnableExtension = [&] ( const AnsiChar *InName ) noexcept -> Bool
    {
        if ( not Available.HasExtension( InName ) )
        {
            return false;
        }
        Application.EnabledExtensions.push_back( InName );
        return true;
    };

    if ( EnableExtension( "VK_KHR_portability_enumeration" ) )
    {
        Application.Flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        Application.bPortabilityEnumerationEnabled = true;
    }

    if ( EnableExtension( VK_KHR_SURFACE_EXTENSION_NAME ) )
    {
        constexpr const AnsiChar *PlatformSurfaceExtensions[] = {
            "VK_KHR_wayland_surface", "VK_KHR_xcb_surface", "VK_KHR_xlib_surface", "VK_EXT_metal_surface", "VK_KHR_win32_surface",
        };

        Algo::ForEach( PlatformSurfaceExtensions, [&] ( const AnsiChar *InName ) noexcept
        {
            if ( EnableExtension( InName ) )
            {
                Application.bSurfaceEnabled = true;
            }
        } );
    }

    Application.bSwapchainColorspaceEnabled = EnableExtension( "VK_EXT_swapchain_colorspace" );

    if ( bEnableDebug )
    {
        if ( Available.HasLayer( "VK_LAYER_KHRONOS_validation" ) )
        {
            Application.EnabledLayers.push_back( "VK_LAYER_KHRONOS_validation" );
        }
        Application.bDebugUtilsEnabled = EnableExtension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }

    return Application;
}

/**
 * Public
 */

VkApplicationInfo Lumen::RHIVulkan::FVulkanApplication::MakeApplicationInfo () const noexcept
{
    VkApplicationInfo ApplicationInfo{};
    ApplicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ApplicationInfo.pApplicationName   = "Lumen";
    ApplicationInfo.applicationVersion = VK_MAKE_VERSION( 1U, 0U, 0U );
    ApplicationInfo.pEngineName        = "Lumen";
    ApplicationInfo.engineVersion      = VK_MAKE_VERSION( 1U, 0U, 0U );
    ApplicationInfo.apiVersion         = VK_API_VERSION_1_4;
    return ApplicationInfo;
}

VkDebugUtilsMessengerCreateInfoEXT Lumen::RHIVulkan::FVulkanApplication::MakeDebugMessengerCreateInfo () const noexcept
{
    VkDebugUtilsMessengerCreateInfoEXT CreateInfo{};
    CreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    CreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    CreateInfo.pfnUserCallback = &BootstrapDebugCallback;
    return CreateInfo;
}
