/**
 * @file VulkanInstance.cpp
 * @brief Implements Vulkan instance.
 */

#include <RHI/Types/RHIError.hpp>

#include <RHIVulkan/Module/VulkanInstance.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <Algo/Any.hpp>
#include <Templates/Vector.hpp>

#include <cstring>

/**
 * Private
 */

namespace
{

constexpr Lumen::UInt32 MaxEnumerationRetries = 4U;

Lumen::TExpected<Lumen::TVector<VkExtensionProperties>, Lumen::RHI::FRHIError> EnumerateInstanceExtensions () noexcept
{
    for ( Lumen::UInt32 Attempt = 0U; Attempt < MaxEnumerationRetries; ++Attempt )
    {
        Lumen::UInt32 Count = 0U;
        LUMEN_VK_TRY( vkEnumerateInstanceExtensionProperties( nullptr, &Count, nullptr ),
                      Lumen::RHI::ERHIErrorCondition::Device );

        Lumen::TVector<VkExtensionProperties> Extensions( Count );
        if ( Count == 0 )
        {
            return Extensions;
        }

        /** INFO: Wait for incomplete */
        LUMEN_VK_TRY_WAIT( vkEnumerateInstanceExtensionProperties( nullptr, &Count, Extensions.data() ),
                           Lumen::RHI::ERHIErrorCondition::Device );
    }

    return Lumen::MakeUnexpected( Lumen::RHI::MakeRHICorrupted(
        Lumen::RHI::ERHIErrorCondition::Device, "vkEnumerateInstanceExtensionProperties remained VK_INCOMPLETE." ) );
}

Lumen::TExpected<Lumen::TVector<VkLayerProperties>, Lumen::RHI::FRHIError> EnumerateInstanceLayers () noexcept
{
    for ( Lumen::UInt32 Attempt = 0U; Attempt < MaxEnumerationRetries; ++Attempt )
    {
        Lumen::UInt32 Count = 0U;
        LUMEN_VK_TRY( vkEnumerateInstanceLayerProperties( &Count, nullptr ), Lumen::RHI::ERHIErrorCondition::Device );

        Lumen::TVector<VkLayerProperties> Layers( Count );
        if ( Count == 0 )
        {
            return Layers;
        }

        /** INFO: Wait for incomplete */
        LUMEN_VK_TRY_WAIT( vkEnumerateInstanceLayerProperties( &Count, Layers.data() ), Lumen::RHI::ERHIErrorCondition::Device );
    }

    return Lumen::MakeUnexpected( Lumen::RHI::MakeRHICorrupted(
        Lumen::RHI::ERHIErrorCondition::Device, "vkEnumerateInstanceExtensionProperties remained VK_INCOMPLETE." ) );
}

Lumen::Bool HasExtension ( const Lumen::TVector<VkExtensionProperties> &InExtensions,
                           const Lumen::AnsiChar *const InName ) noexcept
{
    return Lumen::Algo::AnyOf( InExtensions, [InName] ( const VkExtensionProperties &Extension ) noexcept -> Lumen::Bool
    {
        return strcmp( Extension.extensionName, InName ) == 0;
    } );
}

Lumen::Bool HasLayer ( const Lumen::TVector<VkLayerProperties> &InLayers, const Lumen::AnsiChar *const InName ) noexcept
{
    return Lumen::Algo::AnyOf( InLayers, [InName] ( const VkLayerProperties &Layer ) noexcept -> Lumen::Bool
    {
        return strcmp( Layer.layerName, InName ) == 0;
    } );
}

VKAPI_ATTR VkBool32 VKAPI_CALL BootstrapDebugCallback ( VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                        VkDebugUtilsMessageTypeFlagsEXT,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *,
                                                        void * ) noexcept
{
    return VK_FALSE;
}

} // namespace

/**
 * Static
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanInstance>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanInstance::Create ( const Bool bEnableDebug ) noexcept
{
    const auto EnumerateVersion =
        reinterpret_cast<PFN_vkEnumerateInstanceVersion>( vkGetInstanceProcAddr( nullptr, "vkEnumerateInstanceVersion" ) );
    if ( EnumerateVersion == nullptr )
    {
        return MakeUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Capability, "Vulkan loader does not expose Vulkan 1.4." ) );
    }

    UInt32 LoaderVersion = 0U;
    LUMEN_VK_TRY( EnumerateVersion( &LoaderVersion ), RHI::ERHIErrorCondition::Device );

    if ( LoaderVersion < VK_API_VERSION_1_4 )
    {
        TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Capability,
                                              "Vulkan loader API version is below the required 1.4 baseline." ) );
    }

    const TExpected<TVector<VkExtensionProperties>, RHI::FRHIError> AvailableExtensions = EnumerateInstanceExtensions();
    if ( not AvailableExtensions )
    {
        return MakeUnexpected( AvailableExtensions.Error() );
    }

    const TExpected<TVector<VkLayerProperties>, RHI::FRHIError> AvailableLayers = EnumerateInstanceLayers();
    if ( not AvailableLayers )
    {
        return MakeUnexpected( AvailableLayers.Error() );
    }

    TSharedRef<FVulkanInstance> Instance = MakeShared<FVulkanInstance>();

    TVector<const AnsiChar *> EnabledExtensions;
    TVector<const AnsiChar *> EnabledLayers;
    VkInstanceCreateFlags Flags = 0U;

    if ( HasExtension( *AvailableExtensions, "VK_KHR_portability_enumeration" ) )
    {
        EnabledExtensions.push_back( "VK_KHR_portability_enumeration" );
        Flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        Instance->bPortabilityEnumerationEnabled = true;
    }

    if ( HasExtension( *AvailableExtensions, VK_KHR_SURFACE_EXTENSION_NAME ) )
    {
        EnabledExtensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );

        constexpr const AnsiChar *PlatformSurfaceExtensions[] = {
            "VK_KHR_wayland_surface", "VK_KHR_xcb_surface", "VK_KHR_xlib_surface", "VK_EXT_metal_surface", "VK_KHR_win32_surface",
        };

        for ( const AnsiChar *SurfaceExt : PlatformSurfaceExtensions )
        {
            if ( HasExtension( *AvailableExtensions, SurfaceExt ) )
            {
                EnabledExtensions.push_back( SurfaceExt );
                Instance->bSurfaceEnabled = true;
            }
        }
    }

    if ( HasExtension( *AvailableExtensions, "VK_EXT_swapchain_colorspace" ) )
    {
        EnabledExtensions.push_back( "VK_EXT_swapchain_colorspace" );
        Instance->bSwapchainColorspaceEnabled = true;
    }

    if ( bEnableDebug )
    {
        if ( HasLayer( *AvailableLayers, "VK_LAYER_KHRONOS_validation" ) )
        {
            EnabledLayers.push_back( "VK_LAYER_KHRONOS_validation" );
        }
        if ( HasExtension( *AvailableExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) )
        {
            EnabledExtensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
            Instance->bDebugUtilsEnabled = true;
        }
    }

    VkApplicationInfo AppInfo{};
    AppInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName   = "Lumen";
    AppInfo.applicationVersion = VK_MAKE_VERSION( 1U, 0U, 0U );
    AppInfo.pEngineName        = "Lumen";
    AppInfo.engineVersion      = VK_MAKE_VERSION( 1U, 0U, 0U );
    AppInfo.apiVersion         = VK_API_VERSION_1_3;

    VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo{};
    DebugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    DebugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    DebugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    DebugCreateInfo.pfnUserCallback = &BootstrapDebugCallback;

    VkInstanceCreateInfo CreateInfo{};
    CreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.flags                   = Flags;
    CreateInfo.pApplicationInfo        = &AppInfo;
    CreateInfo.enabledLayerCount       = static_cast<uint32_t>( EnabledLayers.size() );
    CreateInfo.ppEnabledLayerNames     = EnabledLayers.empty() ? nullptr : EnabledLayers.data();
    CreateInfo.enabledExtensionCount   = static_cast<uint32_t>( EnabledExtensions.size() );
    CreateInfo.ppEnabledExtensionNames = EnabledExtensions.empty() ? nullptr : EnabledExtensions.data();
    if ( Instance->bDebugUtilsEnabled )
    {
        CreateInfo.pNext = &DebugCreateInfo;
    }

    const VkResult InstanceResult = vkCreateInstance( &CreateInfo, nullptr, Instance->Instance.ResetAndGetAddressOf() );
    LUMEN_VK_TRY( InstanceResult, RHI::ERHIErrorCondition::Device );

    if ( Instance->bDebugUtilsEnabled )
    {
        const auto CreateMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr( Instance->Instance.Get(), "vkCreateDebugUtilsMessengerEXT" ) );
        const auto DestroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr( Instance->Instance.Get(), "vkDestroyDebugUtilsMessengerEXT" ) );

        if ( CreateMessenger != nullptr and DestroyMessenger != nullptr )
        {
            const FVulkanDebugMessengerDeleter Deleter = { .Instance = Instance->Instance.Get(), .DestroyFn = DestroyMessenger };
            LUMEN_VK_TRY( CreateMessenger( Instance->Instance.Get(), &DebugCreateInfo, nullptr,
                                           Instance->DebugMessenger.ResetAndGetAddressOf( Deleter ) ),
                          RHI::ERHIErrorCondition::Device );
        }
    }

    return Instance;
}

/**
 * Public
 */

VkInstance Lumen::RHIVulkan::FVulkanInstance::GetHandle () const noexcept
{
    return Instance.Get();
}

Lumen::Bool Lumen::RHIVulkan::FVulkanInstance::SupportsSurface () const noexcept
{
    return bSurfaceEnabled;
}

Lumen::Bool Lumen::RHIVulkan::FVulkanInstance::SupportsSwapchainColorspace () const noexcept
{
    return bSwapchainColorspaceEnabled;
}

Lumen::Bool Lumen::RHIVulkan::FVulkanInstance::SupportsDebugUtils () const noexcept
{
    return bDebugUtilsEnabled;
}

Lumen::Bool Lumen::RHIVulkan::FVulkanInstance::SupportsPortabilityEnumeration () const noexcept
{
    return bPortabilityEnumerationEnabled;
}
