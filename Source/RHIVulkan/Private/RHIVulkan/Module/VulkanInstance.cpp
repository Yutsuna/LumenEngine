/**
 * @file VulkanInstance.cpp
 * @brief Implements Vulkan instance and debug messenger ownership.
 */

#include <RHIVulkan/Module/VulkanApplication.hpp>
#include <RHIVulkan/Module/VulkanInstance.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>
#include <vulkan/vulkan_core.h>

/**
 * Private
 */

namespace
{

[[nodiscard]] VkInstanceCreateInfo MakeInstanceCreateInfo ( const Lumen::RHIVulkan::FVulkanApplication &InApplication,
                                                            const VkApplicationInfo &InApplicationInfo,
                                                            const VkDebugUtilsMessengerCreateInfoEXT &InDebugCreateInfo ) noexcept
{
    VkInstanceCreateInfo CreateInfo{};
    CreateInfo.sType                 = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.flags                 = InApplication.Flags;
    CreateInfo.pApplicationInfo      = &InApplicationInfo;
    CreateInfo.enabledLayerCount     = static_cast<Lumen::UInt32>( InApplication.EnabledLayers.size() );
    CreateInfo.ppEnabledLayerNames   = InApplication.EnabledLayers.empty() ? nullptr : InApplication.EnabledLayers.data();
    CreateInfo.enabledExtensionCount = static_cast<Lumen::UInt32>( InApplication.EnabledExtensions.size() );
    CreateInfo.ppEnabledExtensionNames =
        InApplication.EnabledExtensions.empty() ? nullptr : InApplication.EnabledExtensions.data();
    if ( InApplication.bDebugUtilsEnabled )
    {
        CreateInfo.pNext = &InDebugCreateInfo;
    }
    return CreateInfo;
}

} // namespace

/**
 * Static Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanInstance>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanInstance::Create ( const Bool bEnableDebug ) noexcept
{
    LUMEN_EXPECT( Application, FVulkanApplication::Create( bEnableDebug ) );

    TSharedRef<FVulkanInstance> Instance = MakeInstanceFromApplication( Application );

    const VkApplicationInfo ApplicationInfo                  = Application.MakeApplicationInfo();
    const VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = Application.MakeDebugMessengerCreateInfo();

    const VkInstanceCreateInfo CreateInfo = MakeInstanceCreateInfo( Application, ApplicationInfo, DebugCreateInfo );

    LUMEN_VK_TRY( vkCreateInstance( &CreateInfo, nullptr, Instance->Instance.ResetAndGetAddressOf() ),
                  RHI::ERHIErrorCondition::Device );

    if ( Application.bDebugUtilsEnabled )
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

/**
 * Static Private
 */

Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanInstance>
Lumen::RHIVulkan::FVulkanInstance::MakeInstanceFromApplication ( const FVulkanApplication &InApplication ) noexcept
{
    TSharedRef<FVulkanInstance> Instance     = MakeShared<FVulkanInstance>();
    Instance->bSurfaceEnabled                = InApplication.bSurfaceEnabled;
    Instance->bSwapchainColorspaceEnabled    = InApplication.bSwapchainColorspaceEnabled;
    Instance->bDebugUtilsEnabled             = InApplication.bDebugUtilsEnabled;
    Instance->bPortabilityEnumerationEnabled = InApplication.bPortabilityEnumerationEnabled;
    return Instance;
}
