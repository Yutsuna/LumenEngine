/**
 * @file VulkanMemory.cpp
 * @brief Implements Vulkan memory-type selection and native memory allocation.
 */

#include <RHIVulkan/Memory/VulkanMemory.hpp>

#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <Templates/Optional.hpp>

/**
 * Public
 */

VkMemoryPropertyFlags Lumen::RHIVulkan::GetVulkanMemoryRequiredFlags ( const RHI::ERHIMemoryClass InMemoryClass ) noexcept
{
    constexpr VkMemoryPropertyFlags HostStagingFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    switch ( InMemoryClass )
    {
    case RHI::ERHIMemoryClass::DeviceLocal:
        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    case RHI::ERHIMemoryClass::Upload:
        return HostStagingFlags;
    case RHI::ERHIMemoryClass::Readback:
        return HostStagingFlags;
    }
    return 0U;
}

Lumen::TOptional<Lumen::UInt32> Lumen::RHIVulkan::SelectVulkanMemoryTypeIndex (
    const VkPhysicalDevice InPhysicalDevice, const UInt32 InTypeBits, const RHI::ERHIMemoryClass InMemoryClass ) noexcept
{
    VkPhysicalDeviceMemoryProperties Properties{};
    vkGetPhysicalDeviceMemoryProperties( InPhysicalDevice, &Properties );

    const VkMemoryPropertyFlags RequiredFlags = GetVulkanMemoryRequiredFlags( InMemoryClass );
    for ( UInt32 TypeIndex = 0U; TypeIndex < Properties.memoryTypeCount; ++TypeIndex )
    {
        if ( ( InTypeBits & ( 1U << TypeIndex ) ) == 0U )
        {
            continue;
        }
        if ( ( Properties.memoryTypes[TypeIndex].propertyFlags & RequiredFlags ) == RequiredFlags )
        {
            return TOptional<UInt32>( TypeIndex );
        }
    }
    return {};
}

Lumen::TOptional<Lumen::UInt32> Lumen::RHIVulkan::SelectVulkanHeapMemoryTypeIndex (
    const VkPhysicalDevice InPhysicalDevice, const UInt64 InSize, const RHI::ERHIMemoryClass InMemoryClass ) noexcept
{
    VkPhysicalDeviceMemoryProperties Properties{};
    vkGetPhysicalDeviceMemoryProperties( InPhysicalDevice, &Properties );

    const VkMemoryPropertyFlags RequiredFlags = GetVulkanMemoryRequiredFlags( InMemoryClass );
    TOptional<UInt32> Fallback;
    for ( UInt32 TypeIndex = 0U; TypeIndex < Properties.memoryTypeCount; ++TypeIndex )
    {
        const VkMemoryType &Type = Properties.memoryTypes[TypeIndex];
        if ( ( Type.propertyFlags & RequiredFlags ) != RequiredFlags )
        {
            continue;
        }
        if ( Properties.memoryHeaps[Type.heapIndex].size < InSize )
        {
            continue;
        }
        if ( not Fallback.HasValue() )
        {
            Fallback.Emplace( TypeIndex );
        }
        if ( ( Type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) != 0U )
        {
            return TOptional<UInt32>( TypeIndex );
        }
    }
    return Fallback;
}

Lumen::TExpected<Lumen::RHIVulkan::FVulkanDeviceMemoryHandle, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::AllocateVulkanMemory ( const VkPhysicalDevice InPhysicalDevice,
                                         const VkDevice InDevice,
                                         const VkMemoryRequirements &InRequirements,
                                         const RHI::ERHIMemoryClass InMemoryClass ) noexcept
{
    const TOptional<UInt32> MemoryTypeIndex =
        SelectVulkanMemoryTypeIndex( InPhysicalDevice, InRequirements.memoryTypeBits, InMemoryClass );
    if ( not MemoryTypeIndex.HasValue() )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Memory,
                                                     "No Vulkan memory type satisfies the requested memory class." ) );
    }

    VkMemoryAllocateInfo AllocateInfo{};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize  = InRequirements.size;
    AllocateInfo.memoryTypeIndex = *MemoryTypeIndex;

    FVulkanDeviceMemoryHandle Memory;
    LUMEN_VK_TRY( vkAllocateMemory( InDevice, &AllocateInfo, nullptr,
                                    Memory.ResetAndGetAddressOf( FVulkanDeviceMemoryDeleter{ InDevice } ) ),
                  RHI::ERHIErrorCondition::Memory );
    return Memory;
}
