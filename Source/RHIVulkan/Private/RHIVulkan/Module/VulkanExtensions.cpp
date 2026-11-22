/**
 * @file VulkanExtensions.cpp
 * @brief Implements Vulkan extension and layer discovery helpers.
 */

#include <RHIVulkan/Module/VulkanExtensions.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <Algo/Any.hpp>

#include <cstring>
#include <utility>

/**
 * Private
 */

namespace
{

constexpr Lumen::UInt32 MaxEnumerationRetries = 4U;

template <class TProperty, auto Function, class... TArguments>
[[nodiscard]] Lumen::TExpected<Lumen::TVector<TProperty>, Lumen::RHI::FRHIError>
EnumerateProperties ( Lumen::FStringView InIncompleteMessage, TArguments... InArguments ) noexcept
{
    for ( Lumen::UInt32 Attempt = 0U; Attempt < MaxEnumerationRetries; ++Attempt )
    {
        Lumen::UInt32 Count = 0U;
        LUMEN_VK_TRY( Function( InArguments..., &Count, nullptr ), Lumen::RHI::ERHIErrorCondition::Device );

        Lumen::TVector<TProperty> Properties( Count );
        if ( Count == 0U )
        {
            return Properties;
        }

        const VkResult ListResult = Function( InArguments..., &Count, Properties.data() );
        if ( ListResult == VK_SUCCESS )
        {
            Properties.resize( Count );
            return Properties;
        }
        if ( ListResult != VK_INCOMPLETE )
        {
            LUMEN_VK_TRY( ListResult, Lumen::RHI::ERHIErrorCondition::Device );
        }
    }

    return Lumen::MakeUnexpected( Lumen::RHI::MakeRHICorrupted( Lumen::RHI::ERHIErrorCondition::Device, InIncompleteMessage ) );
}

} // namespace

#define LUMEN_VK_ENUMERATE_PROPERTIES( PropertyType, Function, ... )                                                             \
    EnumerateProperties<PropertyType, Function>( LUMEN_VK_RESULT_STR( Function ) " remained VK_INCOMPLETE." __VA_OPT__(, )       \
                                                     __VA_ARGS__ )

/**
 * Static
 */

Lumen::TExpected<Lumen::RHIVulkan::FVulkanExtensions, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanExtensions::Enumerate () noexcept
{
    FVulkanExtensions Available;

    LUMEN_EXPECT( InstanceExtensions,
                  LUMEN_VK_ENUMERATE_PROPERTIES( VkExtensionProperties, vkEnumerateInstanceExtensionProperties, nullptr ) );

    Available.Extensions = std::move( InstanceExtensions );

    LUMEN_EXPECT( InstanceLayers, LUMEN_VK_ENUMERATE_PROPERTIES( VkLayerProperties, vkEnumerateInstanceLayerProperties ) );

    Available.Layers = std::move( InstanceLayers );

    return Available;
}

Lumen::TExpected<Lumen::RHIVulkan::FVulkanExtensions, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanExtensions::Enumerate ( VkPhysicalDevice InPhysicalDevice ) noexcept
{
    FVulkanExtensions Available;

    LUMEN_EXPECT( DeviceExtensions, LUMEN_VK_ENUMERATE_PROPERTIES( VkExtensionProperties, vkEnumerateDeviceExtensionProperties,
                                                                   InPhysicalDevice, nullptr ) );
    Available.Extensions = std::move( DeviceExtensions );

    return Available;
}

#undef LUMEN_VK_ENUMERATE_PROPERTIES

/**
 * Public
 */

Lumen::Bool Lumen::RHIVulkan::FVulkanExtensions::HasExtension ( const AnsiChar *const InName ) const noexcept
{
    return Algo::AnyOf( Extensions, [InName] ( const VkExtensionProperties &Extension ) noexcept -> Bool
    {
        return std::strcmp( Extension.extensionName, InName ) == 0;
    } );
}

Lumen::Bool Lumen::RHIVulkan::FVulkanExtensions::HasLayer ( const AnsiChar *const InName ) const noexcept
{
    return Algo::AnyOf( Layers, [InName] ( const VkLayerProperties &Layer ) noexcept -> Bool
    {
        return std::strcmp( Layer.layerName, InName ) == 0;
    } );
}
