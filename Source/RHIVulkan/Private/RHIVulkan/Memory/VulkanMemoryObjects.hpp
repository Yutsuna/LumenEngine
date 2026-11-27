/**
 * @file VulkanMemoryObjects.hpp
 * @brief Declares Vulkan buffer pool, transient allocator and defragmenter objects.
 */
#pragma once

#include <Memory/SharedPtr.hpp>

#include <RHI/Memory/IRHIBufferPool.hpp>
#include <RHI/Memory/IRHIDefragmenter.hpp>
#include <RHI/Memory/IRHITransientResourceAllocator.hpp>
#include <RHI/Memory/RHITlsfAllocator.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Types/RHIIdentifiers.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

#include <RHIVulkan/Device/VulkanDeviceContext.hpp>
#include <RHIVulkan/Memory/VulkanHeap.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class FVulkanBufferPool
     * @brief Recycles buffers created from the shared native device context.
     */
    class FVulkanBufferPool final : public RHI::IRHIBufferPool
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /** @brief Creates an empty pool bound to InContext. */
        FVulkanBufferPool ( TSharedRef<FVulkanDeviceContext> InContext, RHI::FRHIObjectId InId ) noexcept;
        ~FVulkanBufferPool () noexcept override;

        TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError> Acquire ( const RHI::FRHIBufferDesc &InDesc ) noexcept override;
        RHI::FRHIExpected Release ( TSharedRef<RHI::IRHIBuffer> InBuffer, RHI::FRHITimelineValue InCompletion ) noexcept override;
        void AdvanceCompleted ( RHI::FRHITimelineValue InCompleted ) noexcept override;
        [[nodiscard]] USize GetPendingCount () const noexcept override;

    private:

        struct FEntry
        {
            TSharedRef<RHI::IRHIBuffer> Buffer;
            RHI::FRHITimelineValue Completion;
        };

        [[nodiscard]] RHI::FRHIObjectId NextId () noexcept;

        RHI::FRHIObjectBase Identity;
        TSharedRef<FVulkanDeviceContext> Context;
        RHI::FRHIIdentifierGenerator Identifiers;
        TVector<FEntry> Free;
        TVector<FEntry> Released;
        RHI::FRHITimelineValue CurrentCompleted;
    };

    /**
     * @class FVulkanTransientResourceAllocator
     * @brief Places transient requests in a native-backed arena using public TLSF policy.
     */
    class FVulkanTransientResourceAllocator final : public RHI::IRHITransientResourceAllocator
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /** @brief Creates the allocator with a transient arena heap. */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanTransientResourceAllocator>, RHI::FRHIError>
        Create ( TSharedRef<FVulkanDeviceContext> InContext, RHI::FRHIObjectId InId, RHI::FRHIObjectId InHeapId ) noexcept;

        FVulkanTransientResourceAllocator ( TSharedRef<FVulkanDeviceContext> InContext,
                                            TSharedRef<FVulkanHeap> InHeap,
                                            RHI::FRHIObjectId InId ) noexcept;
        ~FVulkanTransientResourceAllocator () noexcept override;

        void BeginBatch () noexcept override;
        TExpected<TVector<RHI::FRHITransientAllocation>, RHI::FRHIError>
        Allocate ( const RHI::FRHITransientBatch &InBatch ) noexcept override;
        void EndBatch () noexcept override;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FVulkanDeviceContext> Context;
        TSharedRef<FVulkanHeap> Heap;
        RHI::FRHITlsfAllocator Allocator;
        Bool bInBatch = false;
    };

    /**
     * @class FVulkanDefragmenter
     * @brief Builds conservative compaction plans for live native allocations.
     */
    class FVulkanDefragmenter final : public RHI::IRHIDefragmenter
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        explicit FVulkanDefragmenter ( RHI::FRHIObjectId InId ) noexcept;
        ~FVulkanDefragmenter () noexcept override;

        TExpected<RHI::FRHIDefragPlan, RHI::FRHIError>
        BuildPlan ( TSpan<const RHI::FRHIAllocation> InAllocations ) noexcept override;
        RHI::FRHIExpected Execute ( const RHI::FRHIDefragPlan &InPlan ) noexcept override;

    private:

        RHI::FRHIObjectBase Identity;
    };

} // namespace RHIVulkan

} // namespace Lumen
