/**
 * @file VulkanBuffer.hpp
 * @brief Declares a Vulkan buffer resource with dedicated or placed memory.
 */
#pragma once

#include <Memory/Span.hpp>

#include <HAL/PlatformMutex.hpp>
#include <Memory/SharedPtr.hpp>

#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Memory/RHIResourceRequirements.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

#include <RHIVulkan/Device/VulkanDeviceContext.hpp>
#include <RHIVulkan/Types/VulkanHandle.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class FVulkanBuffer
     * @brief Native VkBuffer with a dedicated allocation or a retained placed heap range.
     */
    class FVulkanBuffer final : public RHI::IRHIBuffer
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /**
         * @brief Creates a dedicated buffer and binds a native allocation selected for its memory class.
         * @param InContext Shared native device context.
         * @param InDesc Validated buffer descriptor.
         * @param InId Identifier assigned by the owning device.
         * @return The buffer, or a structured error with the Resource/Memory/Limit condition.
         */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanBuffer>, RHI::FRHIError>
        Create ( TSharedRef<FVulkanDeviceContext> InContext, const RHI::FRHIBufferDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        /**
         * @brief Creates a buffer placed in an existing heap range, retaining the heap.
         * @param InContext Shared native device context.
         * @param InDesc Validated buffer descriptor.
         * @param InHeap Heap owning the native memory.
         * @param InAllocation Validated range within the heap.
         * @param InId Identifier assigned by the owning device.
         * @return The buffer, or a structured error with the Resource/Memory condition.
         */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanBuffer>, RHI::FRHIError>
        CreatePlaced ( TSharedRef<FVulkanDeviceContext> InContext,
                       const RHI::FRHIBufferDesc &InDesc,
                       TSharedRef<RHI::IRHIHeap> InHeap,
                       const RHI::FRHIAllocation &InAllocation,
                       RHI::FRHIObjectId InId ) noexcept;

        ~FVulkanBuffer () noexcept override;

        const RHI::FRHIBufferDesc &GetDesc () const noexcept override;
        TExpected<TSpan<Byte>, RHI::FRHIError> Map ( UInt64 InOffset, UInt64 InSize ) noexcept override;
        void Unmap () noexcept override;
        const RHI::FRHIResourceRequirements &GetRequirements () const noexcept override;
        RHI::ERHIResourceUsage GetUsage () const noexcept override;
        RHI::ERHIResourceState GetInitialState () const noexcept override;
        RHI::FRHIResourceLifetime GetLifetime () const noexcept override;

        /** @return Native VkBuffer handle used by texture views and later command translation. */
        [[nodiscard]] VkBuffer GetNativeBuffer () const noexcept;

        /** @return True when this buffer is bound into a retained heap. */
        [[nodiscard]] Bool IsPlaced () const noexcept;

        FVulkanBuffer ( TSharedRef<FVulkanDeviceContext> InContext,
                        const RHI::FRHIBufferDesc &InDesc,
                        RHI::FRHIObjectId InId ) noexcept;

        [[nodiscard]] VkDeviceMemory GetNativeMemory () const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FVulkanDeviceContext> Context;
        RHI::FRHIBufferDesc Desc;
        RHI::FRHIResourceRequirements Requirements;
        FVulkanBufferHandle Buffer;
        FVulkanDeviceMemoryHandle OwnMemory;
        TSharedPtr<RHI::IRHIHeap> Heap;
        RHI::FRHIAllocation Allocation;
        UInt64 MappingBaseOffset = 0ULL;
        Byte *MappedBase         = nullptr;
        HAL::FPlatformRecursiveSharedMutex MappingMutex;
    };

} // namespace RHIVulkan

} // namespace Lumen
