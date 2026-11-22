/**
 * @file VulkanDeviceContext.hpp
 * @brief Declares shared native Vulkan device lifetime and loss state without retaining child RHI objects.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <RHI/Device/RHIDeviceStatus.hpp>
#include <RHI/Types/RHICapabilities.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHILimits.hpp>
#include <RHI/Types/RHIQueueType.hpp>

#include <RHIVulkan/Types/VulkanHandle.hpp>

#include <HAL/PlatformMutex.hpp>
#include <Memory/SharedPtr.hpp>
#include <String/StringView.hpp>
#include <Templates/Optional.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    class FVulkanInstance;

    /**
     * @class FVulkanDeviceContext
     * @brief Owns native VkDevice, queues, and status; retained by children without creating object cycles.
     */
    class FVulkanDeviceContext final : public FNonCopyable, public FNonMovable
    {
    public:

        FVulkanDeviceContext ( TSharedRef<FVulkanInstance> InInstance,
                               VkPhysicalDevice InPhysicalDevice,
                               FVulkanDeviceHandle InDevice,
                               RHI::FRHICapabilities InCapabilities,
                               RHI::FRHILimits InLimits ) noexcept;
        ~FVulkanDeviceContext () noexcept;

        /** @return Owning Vulkan instance wrapper. */
        [[nodiscard]] const TSharedRef<FVulkanInstance> &GetInstance () const noexcept;

        /** @return Native VkPhysicalDevice. */
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice () const noexcept;

        /** @return Native VkDevice handle. */
        [[nodiscard]] VkDevice GetDevice () const noexcept;

        /** @return Negotiated RHI capabilities. */
        [[nodiscard]] const RHI::FRHICapabilities &GetCapabilities () const noexcept;

        /** @return Negotiated RHI limits. */
        [[nodiscard]] const RHI::FRHILimits &GetLimits () const noexcept;

        /** @return Current operational device status. */
        [[nodiscard]] RHI::ERHIDeviceStatus GetStatus () const noexcept;

        /** @return DeviceLost or DeviceClosed when the device is no longer Ready. */
        [[nodiscard]] TOptional<RHI::FRHIError> GetUnavailableError ( FStringView InOperation ) const noexcept;

        /** @brief Monotonically transitions status to Lost. */
        void MarkLost () noexcept;

        /** @brief Monotonically transitions status to Closed when not already Lost. */
        void MarkClosed () noexcept;

        /** @brief Registers a native queue handle and family index created for InType. */
        void SetQueue ( RHI::ERHIQueueType InType, UInt32 InFamilyIndex, VkQueue InQueue ) noexcept;

        /** @return Native VkQueue handle for InType, or VK_NULL_HANDLE if not created. */
        [[nodiscard]] VkQueue GetNativeQueue ( RHI::ERHIQueueType InType ) const noexcept;

        /** @return Queue family index for InType, or empty if not created. */
        [[nodiscard]] TOptional<UInt32> GetQueueFamilyIndex ( RHI::ERHIQueueType InType ) const noexcept;

        /** @return Queue submission/idle mutex for InType. */
        [[nodiscard]] HAL::FPlatformRecursiveSharedMutex &GetQueueMutex ( RHI::ERHIQueueType InType ) noexcept;

    private:

        struct FNativeQueueSlot
        {
            VkQueue Handle = VK_NULL_HANDLE;
            TOptional<UInt32> FamilyIndex;
            HAL::FPlatformRecursiveSharedMutex Mutex;
        };

        TSharedRef<FVulkanInstance> Instance;
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        FVulkanDeviceHandle Device;
        RHI::FRHICapabilities Capabilities;
        RHI::FRHILimits Limits;
        TAtomic<RHI::ERHIDeviceStatus> Status{ RHI::ERHIDeviceStatus::Ready };
        FNativeQueueSlot QueueSlots[static_cast<USize>( RHI::ERHIQueueType::Count )];
    };

} // namespace RHIVulkan

} // namespace Lumen
