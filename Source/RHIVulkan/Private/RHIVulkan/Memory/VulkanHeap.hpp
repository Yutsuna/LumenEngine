/**
 * @file VulkanHeap.hpp
 * @brief Declares a Vulkan device-memory heap exposing placement and CPU mapping.
 */
#pragma once

#include <Memory/Span.hpp>

#include <HAL/PlatformMutex.hpp>
#include <Memory/SharedPtr.hpp>

#include <RHI/Memory/IRHIHeap.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

#include <RHIVulkan/Device/VulkanDeviceContext.hpp>
#include <RHIVulkan/Types/VulkanHandle.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class FVulkanHeap
     * @brief Logical heap backed by one native VkDeviceMemory allocation.
     */
    class FVulkanHeap final : public RHI::IRHIHeap
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /**
         * @brief Creates a heap after validating the descriptor and selecting a native memory type.
         * @param InContext Shared native device context.
         * @param InDesc Validated heap descriptor.
         * @param InId Identifier assigned by the owning device.
         * @return The heap, or a structured error with the Memory condition.
         */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanHeap>, RHI::FRHIError>
        Create ( TSharedRef<FVulkanDeviceContext> InContext, const RHI::FRHIHeapDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        ~FVulkanHeap () noexcept override;

        const RHI::FRHIHeapDesc &GetDesc () const noexcept override;
        TExpected<TSpan<Byte>, RHI::FRHIError> Map ( UInt64 InOffset, UInt64 InSize ) noexcept override;
        void Unmap () noexcept override;

        /** @return Native memory handle used for placed resource binding. */
        [[nodiscard]] VkDeviceMemory GetNativeMemory () const noexcept;

        /** @return Native memory type index selected at creation. */
        [[nodiscard]] UInt32 GetMemoryTypeIndex () const noexcept;

        FVulkanHeap ( TSharedRef<FVulkanDeviceContext> InContext,
                      const RHI::FRHIHeapDesc &InDesc,
                      RHI::FRHIObjectId InId,
                      UInt32 InMemoryTypeIndex ) noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FVulkanDeviceContext> Context;
        RHI::FRHIHeapDesc Desc;
        FVulkanDeviceMemoryHandle Memory;
        UInt32 MemoryTypeIndex = 0U;
        Byte *MappedBase       = nullptr;
        HAL::FPlatformRecursiveSharedMutex MappingMutex;
    };

} // namespace RHIVulkan

} // namespace Lumen
