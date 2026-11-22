/**
 * @file VulkanQueue.hpp
 * @brief Declares the Vulkan IRHIQueue implementation (V2 baseline; submission implemented in V4).
 */

#pragma once

#include <Memory/SharedPtr.hpp>

#include <RHI/Queue/IRHIQueue.hpp>

#include <RHIVulkan/Device/VulkanDeviceContext.hpp>
#include <RHIVulkan/Types/VulkanObject.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class FVulkanQueue
     * @brief Serialized Vulkan queue wrapper implementing identity, idle, and timeline waits.
     */
    class FVulkanQueue final : public RHI::IRHIQueue
    {
    public:

        FVulkanQueue ( RHI::FRHIObjectId InId,
                       TSharedRef<FVulkanDeviceContext> InContext,
                       RHI::ERHIQueueType InType,
                       UInt32 InFamilyIndex,
                       VkQueue InQueue ) noexcept;
        ~FVulkanQueue () noexcept override = default;

        LUMEN_VK_OBJECT_FORWARDERS( ObjectBase )

        /** @return Fixed queue type. */
        [[nodiscard]] RHI::ERHIQueueType GetType () const noexcept override;

        /** @return Latest monotonic completed timeline value. */
        [[nodiscard]] RHI::FRHITimelineValue GetCompletedValue () const noexcept override;

        /** @brief Submits command lists and semaphore operations (V4). */
        TExpected<void, RHI::FRHIError> Submit ( const RHI::FRHISubmitDesc &InDesc ) noexcept override;

        /** @brief Blocks until all queued Vulkan work on this queue finishes. */
        TExpected<void, RHI::FRHIError> WaitIdle () noexcept override;

        /** @brief Waits or polls for a timeline completion value. */
        TExpected<void, RHI::FRHIError> Wait ( RHI::FRHITimelineValue InValue, Float64 InTimeoutSeconds ) noexcept override;

    private:

        RHI::FRHIObjectBase ObjectBase;
        TSharedRef<FVulkanDeviceContext> Context;
        RHI::ERHIQueueType QueueType;
        UInt32 FamilyIndex  = 0U;
        VkQueue NativeQueue = VK_NULL_HANDLE;
        TAtomic<UInt64> CompletedValue{ 0ULL };
    };

} // namespace RHIVulkan

} // namespace Lumen
