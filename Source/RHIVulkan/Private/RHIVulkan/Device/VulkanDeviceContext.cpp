/**
 * @file VulkanDeviceContext.cpp
 * @brief Implements shared native Vulkan device context lifecycle and loss transitions.
 */

#include <RHIVulkan/Device/VulkanDeviceContext.hpp>
#include <RHIVulkan/Module/VulkanInstance.hpp>

/**
 * Public
 */

Lumen::RHIVulkan::FVulkanDeviceContext::FVulkanDeviceContext ( TSharedRef<FVulkanInstance> InInstance,
                                                               VkPhysicalDevice InPhysicalDevice,
                                                               FVulkanDeviceHandle InDevice,
                                                               RHI::FRHICapabilities InCapabilities,
                                                               RHI::FRHILimits InLimits ) noexcept
    : Instance( static_cast<TSharedRef<FVulkanInstance> &&>( InInstance ) ), PhysicalDevice( InPhysicalDevice ),
      Device( static_cast<FVulkanDeviceHandle &&>( InDevice ) ), Capabilities( InCapabilities ), Limits( InLimits )
{
    /* */
}

Lumen::RHIVulkan::FVulkanDeviceContext::~FVulkanDeviceContext () noexcept
{
    if ( Device.IsValid() and GetStatus() != RHI::ERHIDeviceStatus::Lost )
    {
        ( void )vkDeviceWaitIdle( Device.Get() );
    }
    MarkClosed();
}

const Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanInstance> &Lumen::RHIVulkan::FVulkanDeviceContext::GetInstance () const noexcept
{
    return Instance;
}

VkPhysicalDevice Lumen::RHIVulkan::FVulkanDeviceContext::GetPhysicalDevice () const noexcept
{
    return PhysicalDevice;
}

VkDevice Lumen::RHIVulkan::FVulkanDeviceContext::GetDevice () const noexcept
{
    return Device.Get();
}

const Lumen::RHI::FRHICapabilities &Lumen::RHIVulkan::FVulkanDeviceContext::GetCapabilities () const noexcept
{
    return Capabilities;
}

const Lumen::RHI::FRHILimits &Lumen::RHIVulkan::FVulkanDeviceContext::GetLimits () const noexcept
{
    return Limits;
}

Lumen::RHI::ERHIDeviceStatus Lumen::RHIVulkan::FVulkanDeviceContext::GetStatus () const noexcept
{
    return Status.load();
}

Lumen::TOptional<Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDeviceContext::GetUnavailableError ( FStringView InOperation ) const noexcept
{
    const RHI::ERHIDeviceStatus Current = Status.load();
    if ( Current == RHI::ERHIDeviceStatus::Ready )
    {
        return {};
    }

    const RHI::ERHIErrorCode Code =
        ( Current == RHI::ERHIDeviceStatus::Lost ) ? RHI::ERHIErrorCode::DeviceLost : RHI::ERHIErrorCode::DeviceClosed;

    const RHI::FRHIError Error( Code, RHI::ERHIErrorCondition::Device, "Vulkan device is unavailable." );
    return TOptional<RHI::FRHIError>( InOperation.empty() ? Error : Error.WithContext( InOperation ) );
}

void Lumen::RHIVulkan::FVulkanDeviceContext::MarkLost () noexcept
{
    Status.store( RHI::ERHIDeviceStatus::Lost );
}

void Lumen::RHIVulkan::FVulkanDeviceContext::MarkClosed () noexcept
{
    RHI::ERHIDeviceStatus Expected = RHI::ERHIDeviceStatus::Ready;
    ( void )Status.compare_exchange_strong( Expected, RHI::ERHIDeviceStatus::Closed );
}

void Lumen::RHIVulkan::FVulkanDeviceContext::SetQueue ( RHI::ERHIQueueType InType,
                                                        UInt32 InFamilyIndex,
                                                        VkQueue InQueue ) noexcept
{
    const USize Index = static_cast<USize>( InType );
    if ( Index < static_cast<USize>( RHI::ERHIQueueType::Count ) )
    {
        QueueSlots[Index].FamilyIndex.Emplace( InFamilyIndex );
        QueueSlots[Index].Handle = InQueue;
    }
}

VkQueue Lumen::RHIVulkan::FVulkanDeviceContext::GetNativeQueue ( RHI::ERHIQueueType InType ) const noexcept
{
    const USize Index = static_cast<USize>( InType );
    if ( Index < static_cast<USize>( RHI::ERHIQueueType::Count ) )
    {
        return QueueSlots[Index].Handle;
    }
    return VK_NULL_HANDLE;
}

Lumen::TOptional<Lumen::UInt32>
Lumen::RHIVulkan::FVulkanDeviceContext::GetQueueFamilyIndex ( RHI::ERHIQueueType InType ) const noexcept
{
    const USize Index = static_cast<USize>( InType );
    if ( Index < static_cast<USize>( RHI::ERHIQueueType::Count ) )
    {
        return QueueSlots[Index].FamilyIndex;
    }
    return {};
}

Lumen::HAL::FPlatformRecursiveSharedMutex &
Lumen::RHIVulkan::FVulkanDeviceContext::GetQueueMutex ( RHI::ERHIQueueType InType ) noexcept
{
    const USize Index = static_cast<USize>( InType );
    return QueueSlots[Index].Mutex;
}
