/**
 * @file VulkanMemoryTests.cpp
 * @brief Integration tests for Vulkan heaps and staging allocators.
 */

#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Memory/IRHIBufferPool.hpp>
#include <RHI/Memory/IRHIDefragmenter.hpp>
#include <RHI/Memory/IRHIHeap.hpp>
#include <RHI/Memory/IRHIReadbackAllocator.hpp>
#include <RHI/Memory/IRHITransientResourceAllocator.hpp>
#include <RHI/Memory/IRHIUploadAllocator.hpp>
#include <RHI/Memory/RHIDefragPlan.hpp>
#include <RHI/Memory/RHIHeapDesc.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHIVulkan/RHIVulkanModule.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    namespace
    {

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> CreateTestDevice () noexcept
        {
            const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
            if ( not Module )
            {
                return TUnexpected( Module.Error() );
            }
            const TExpected<TVector<RHI::FRHIAdapterInfo>, RHI::FRHIError> Adapters = ( *Module )->EnumerateAdapters();
            if ( not Adapters )
            {
                return TUnexpected( Adapters.Error() );
            }
            if ( Adapters->empty() )
            {
                return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Device, "No Vulkan adapter available." ) );
            }
            RHI::FRHIDeviceDesc Desc;
            Desc.AdapterId.Emplace( ( *Adapters )[0].AdapterId );
            return ( *Module )->CreateDevice( Desc );
        }

    } // namespace

    LUMEN_TEST( VulkanHeap, MapsCpuVisibleRangesAndRejectsInvalidOnes )
    {
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = CreateTestDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const TExpected<TSharedRef<RHI::IRHIHeap>, RHI::FRHIError> Heap =
            ( *Device )->CreateHeap( { .Size = 4096ULL, .MemoryClass = RHI::ERHIMemoryClass::Upload } );
        LUMEN_CHECK_TRUE( Heap.HasValue() );

        const TExpected<TSpan<Byte>, RHI::FRHIError> Mapped = ( *Heap )->Map( 64ULL, 32ULL );
        LUMEN_CHECK_TRUE( Mapped.HasValue() );
        LUMEN_CHECK_EQ( Mapped->size(), 32U );
        ( *Mapped )[0] = Byte{ 0xA5U };
        ( *Heap )->Unmap();

        const TExpected<TSpan<Byte>, RHI::FRHIError> Again = ( *Heap )->Map( 64ULL, 1ULL );
        LUMEN_CHECK_TRUE( Again.HasValue() );
        LUMEN_CHECK_EQ( static_cast<UInt32>( ( *Again )[0] ), 0xA5U );
        ( *Heap )->Unmap();

        const TExpected<TSpan<Byte>, RHI::FRHIError> Outside = ( *Heap )->Map( 4095ULL, 2ULL );
        LUMEN_CHECK_HAS_ERROR( Outside );
        LUMEN_CHECK_EQ( Outside.Error().Condition, RHI::ERHIErrorCondition::Memory );
    }

    LUMEN_TEST( VulkanStaging, UploadAndReadbackRingsExposeUsableBuffers )
    {
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = CreateTestDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const TExpected<TSharedRef<RHI::IRHIUploadAllocator>, RHI::FRHIError> Upload =
            ( *Device )->CreateUploadAllocator( 256ULL );
        const TExpected<TSharedRef<RHI::IRHIReadbackAllocator>, RHI::FRHIError> Readback =
            ( *Device )->CreateReadbackAllocator( 256ULL );
        LUMEN_CHECK_TRUE( Upload.HasValue() );
        LUMEN_CHECK_TRUE( Readback.HasValue() );

        const TExpected<RHI::FRHIUploadAllocation, RHI::FRHIError> Allocation =
            ( *Upload )->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_TRUE( Allocation.HasValue() );
        LUMEN_CHECK_EQ( Allocation->MappedData.size(), 32U );
        Allocation->MappedData[0] = Byte{ 0x5AU };
        const TExpected<TSpan<Byte>, RHI::FRHIError> Shared =
            Allocation->Buffer->Map( Allocation->Allocation.Offset, Allocation->Allocation.Size );
        LUMEN_CHECK_TRUE( Shared.HasValue() );
        LUMEN_CHECK_EQ( static_cast<UInt32>( ( *Shared )[0] ), 0x5AU );
        Allocation->Buffer->Unmap();

        const TExpected<RHI::FRHIAllocation, RHI::FRHIError> ReadbackAllocation =
            ( *Readback )->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_TRUE( ReadbackAllocation.HasValue() );
        const TExpected<RHI::FRHIReadbackAllocation, RHI::FRHIError> Mapped = ( *Readback )->Map( *ReadbackAllocation );
        LUMEN_CHECK_TRUE( Mapped.HasValue() );
        LUMEN_CHECK_EQ( Mapped->MappedData.size(), 32U );
        const TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError> Buffer = ( *Readback )->GetBuffer( *ReadbackAllocation );
        LUMEN_CHECK_TRUE( Buffer.HasValue() );
        LUMEN_CHECK_EQ( ( *Buffer )->GetDesc().MemoryClass, RHI::ERHIMemoryClass::Readback );

        LUMEN_CHECK_HAS_ERROR( ( *Readback )->Map( {} ) );
        LUMEN_CHECK_TRUE( ( *Upload )->Free( Allocation->Allocation ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( ( *Upload )->Free( Allocation->Allocation ) );
        LUMEN_CHECK_TRUE( ( *Readback )->Free( *ReadbackAllocation ).HasValue() );
    }

    LUMEN_TEST( VulkanBufferPool, ReusesBuffersOnlyAfterCompletion )
    {
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = CreateTestDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const TSharedRef<RHI::IRHIBufferPool> Pool = *( *Device )->CreateBufferPool();
        const RHI::FRHIBufferDesc Desc{ .Size = 64ULL, .Usage = RHI::ERHIResourceUsage::CopySource };
        const TSharedRef<RHI::IRHIBuffer> First = *Pool->Acquire( Desc );
        const RHI::FRHIObjectId FirstId         = First->GetId();
        LUMEN_CHECK_TRUE( Pool->Release( First, RHI::FRHITimelineValue{ 7ULL } ).HasValue() );
        LUMEN_CHECK_EQ( Pool->GetPendingCount(), 1U );
        Pool->AdvanceCompleted( RHI::FRHITimelineValue{ 6ULL } );
        const TSharedRef<RHI::IRHIBuffer> Before = *Pool->Acquire( Desc );
        LUMEN_CHECK_NOT_EQ( Before->GetId().GetValue(), FirstId.GetValue() );
        Pool->AdvanceCompleted( RHI::FRHITimelineValue{ 7ULL } );
        const TSharedRef<RHI::IRHIBuffer> Reused = *Pool->Acquire( Desc );
        LUMEN_CHECK_EQ( Reused->GetId().GetValue(), FirstId.GetValue() );
        LUMEN_CHECK_EQ( Pool->GetPendingCount(), 0U );
    }

    LUMEN_TEST( VulkanTransient, PlacesDisjointAndOverlappingLifetimes )
    {
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = CreateTestDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const TSharedRef<RHI::IRHITransientResourceAllocator> Allocator = *( *Device )->CreateTransientResourceAllocator();

        RHI::FRHITransientBatch Batch;
        Batch.Requests.push_back( { .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                    .Lifetime     = { .Begin = 0ULL, .End = 1ULL } } );
        Batch.Requests.push_back( { .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                    .Lifetime     = { .Begin = 1ULL, .End = 2ULL } } );
        Allocator->BeginBatch();
        const TExpected<TVector<RHI::FRHITransientAllocation>, RHI::FRHIError> Disjoint = Allocator->Allocate( Batch );
        Allocator->EndBatch();
        LUMEN_CHECK_TRUE( Disjoint.HasValue() );
        LUMEN_CHECK_EQ( Disjoint->size(), 2U );

        Batch.Requests[1].Lifetime = { .Begin = 0ULL, .End = 2ULL };
        Allocator->BeginBatch();
        const TExpected<TVector<RHI::FRHITransientAllocation>, RHI::FRHIError> Result = Allocator->Allocate( Batch );
        Allocator->EndBatch();
        LUMEN_CHECK_TRUE( Result.HasValue() );
        LUMEN_CHECK_NOT_EQ( ( *Result )[0].Allocation.Offset, ( *Result )[1].Allocation.Offset );
    }

    LUMEN_TEST( VulkanDefragmenter, BuildsPlanButDefersNativeMoves )
    {
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = CreateTestDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const TSharedRef<RHI::IRHIDefragmenter> Defragmenter = *( *Device )->CreateDefragmenter();
        const RHI::FRHIAllocation First{ .Id = RHI::FRHIAllocationId{ 1ULL }, .Offset = 128ULL, .Size = 64ULL };
        const RHI::FRHIAllocation Second{ .Id = RHI::FRHIAllocationId{ 2ULL }, .Offset = 256ULL, .Size = 64ULL };
        const RHI::FRHIAllocation Inputs[] = { First, Second };
        const TExpected<RHI::FRHIDefragPlan, RHI::FRHIError> Plan =
            Defragmenter->BuildPlan( TSpan<const RHI::FRHIAllocation>( Inputs, 2U ) );
        LUMEN_CHECK_TRUE( Plan.HasValue() );
        LUMEN_CHECK_TRUE( Plan->Validate().HasValue() );
        LUMEN_CHECK_EQ( Plan->Moves.size(), 2U );
        LUMEN_CHECK_EQ( Plan->Moves[1].Destination.Offset, 64ULL );
        LUMEN_CHECK_HAS_ERROR( Defragmenter->Execute( *Plan ) );
    }

} // namespace RHIVulkan

} // namespace Lumen
