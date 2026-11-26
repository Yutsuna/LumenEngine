/**
 * @file VulkanStagingAllocator.hpp
 * @brief Declares Vulkan upload and readback allocators backed by persistent rings.
 */
#pragma once

#include <Memory/SharedPtr.hpp>

#include <RHI/Memory/IRHIReadbackAllocator.hpp>
#include <RHI/Memory/IRHIUploadAllocator.hpp>
#include <RHI/Memory/RHIRingAllocator.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

#include <RHIVulkan/Device/VulkanDeviceContext.hpp>
#include <RHIVulkan/Resources/VulkanBuffer.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class FVulkanStagingRing
     * @brief Shared ring bookkeeping over one persistently mapped staging buffer.
     */
    class FVulkanStagingRing final
    {
    public:

        FVulkanStagingRing ( TSharedRef<FVulkanBuffer> InBuffer, RHI::ERHIMemoryClass InMemoryClass ) noexcept;

        /** @brief Reserves a validated range of the staging buffer. */
        [[nodiscard]] TExpected<RHI::FRHIAllocation, RHI::FRHIError> Allocate ( const RHI::FRHIAllocationDesc &InDesc ) noexcept;

        /** @brief Releases a previously reserved range. */
        [[nodiscard]] RHI::FRHIExpected Free ( const RHI::FRHIAllocation &InAllocation ) noexcept;

        /** @brief Returns a writable or readable span for a live allocation. */
        [[nodiscard]] TExpected<TSpan<Byte>, RHI::FRHIError> GetMappedData ( const RHI::FRHIAllocation &InAllocation ) noexcept;

        /** @return The staging buffer used as a copy source or destination. */
        [[nodiscard]] const TSharedRef<FVulkanBuffer> &GetBuffer () const noexcept;

        /** @return True when the allocation belongs to this ring and its memory class. */
        [[nodiscard]] Bool Owns ( const RHI::FRHIAllocation &InAllocation ) const noexcept;

    private:

        TSharedRef<FVulkanBuffer> Buffer;
        RHI::FRHIRingAllocator Allocator;
        RHI::ERHIMemoryClass MemoryClass = RHI::ERHIMemoryClass::Upload;
    };

    /**
     * @class FVulkanUploadAllocator
     * @brief CPU-writable staging ring for later GPU copies.
     */
    class FVulkanUploadAllocator final : public RHI::IRHIUploadAllocator
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /**
         * @brief Creates the staging buffer and the upload allocator.
         * @param InContext Shared native device context.
         * @param InCapacity Byte capacity of the persistent ring.
         * @param InId Allocator identifier.
         * @param InBufferId Identifier assigned to the staging buffer.
         * @return The allocator, or a structured error with the Memory condition.
         */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanUploadAllocator>, RHI::FRHIError>
        Create ( TSharedRef<FVulkanDeviceContext> InContext,
                 UInt64 InCapacity,
                 RHI::FRHIObjectId InId,
                 RHI::FRHIObjectId InBufferId ) noexcept;

        FVulkanUploadAllocator ( TSharedRef<FVulkanBuffer> InBuffer, RHI::FRHIObjectId InId ) noexcept;
        ~FVulkanUploadAllocator () noexcept override;

        TExpected<RHI::FRHIUploadAllocation, RHI::FRHIError> Allocate ( const RHI::FRHIAllocationDesc &InDesc ) noexcept override;
        RHI::FRHIExpected Free ( const RHI::FRHIAllocation &InAllocation ) noexcept override;

    private:

        RHI::FRHIObjectBase Identity;
        FVulkanStagingRing Ring;
    };

    /**
     * @class FVulkanReadbackAllocator
     * @brief GPU-readback ring whose ranges are recycled after an explicit free.
     */
    class FVulkanReadbackAllocator final : public RHI::IRHIReadbackAllocator
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /**
         * @brief Creates the staging buffer and the readback allocator.
         * @param InContext Shared native device context.
         * @param InCapacity Byte capacity of the persistent ring.
         * @param InId Allocator identifier.
         * @param InBufferId Identifier assigned to the staging buffer.
         * @return The allocator, or a structured error with the Memory condition.
         */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanReadbackAllocator>, RHI::FRHIError>
        Create ( TSharedRef<FVulkanDeviceContext> InContext,
                 UInt64 InCapacity,
                 RHI::FRHIObjectId InId,
                 RHI::FRHIObjectId InBufferId ) noexcept;

        FVulkanReadbackAllocator ( TSharedRef<FVulkanBuffer> InBuffer, RHI::FRHIObjectId InId ) noexcept;
        ~FVulkanReadbackAllocator () noexcept override;

        TExpected<RHI::FRHIAllocation, RHI::FRHIError> Allocate ( const RHI::FRHIAllocationDesc &InDesc ) noexcept override;
        TExpected<RHI::FRHIReadbackAllocation, RHI::FRHIError> Map ( const RHI::FRHIAllocation &InAllocation ) noexcept override;
        TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>
        GetBuffer ( const RHI::FRHIAllocation &InAllocation ) noexcept override;
        RHI::FRHIExpected Free ( const RHI::FRHIAllocation &InAllocation ) noexcept override;

    private:

        RHI::FRHIObjectBase Identity;
        FVulkanStagingRing Ring;
    };

} // namespace RHIVulkan

} // namespace Lumen
