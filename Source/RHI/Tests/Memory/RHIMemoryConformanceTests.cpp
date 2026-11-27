/**
 * @file RHIMemoryConformanceTests.cpp
 * @brief Runs portable heap, staging, and transient allocation contracts.
 */

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>

#include <RHI/Memory/IRHIReadbackAllocator.hpp>
#include <RHI/Memory/IRHITransientResourceAllocator.hpp>
#include <RHI/Memory/IRHIUploadAllocator.hpp>
#include <RHI/Memory/RHIHeapDesc.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIMemoryConformance, CreatesHeapsAndStagingAllocators )
        {
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            LUMEN_CHECK_TRUE( ( *Device )->CreateHeap( { .Size = 4096ULL, .MemoryClass = ERHIMemoryClass::Upload } ).HasValue() );

            const TExpected<TSharedRef<IRHIUploadAllocator>, FRHIError> Upload = ( *Device )->CreateUploadAllocator( 4096ULL );
            LUMEN_CHECK_TRUE( Upload.HasValue() );

            const TExpected<FRHIUploadAllocation, FRHIError> Allocation =
                ( *Upload )->Allocate( { .Size = 64ULL, .Alignment = 16ULL } );
            LUMEN_CHECK_TRUE( Allocation.HasValue() );
            LUMEN_CHECK_EQ( Allocation->MappedData.size(), 64U );
            LUMEN_CHECK_TRUE( ( *Upload )->Free( Allocation->Allocation ).HasValue() );
        }

        LUMEN_TEST( RHIMemoryConformance, RejectsInvalidStagingOperationsAndDoubleFree )
        {
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            const TExpected<TSharedRef<IRHIUploadAllocator>, FRHIError> Upload = ( *Device )->CreateUploadAllocator( 256ULL );
            LUMEN_CHECK_TRUE( Upload.HasValue() );
            const TExpected<FRHIUploadAllocation, FRHIError> Empty = ( *Upload )->Allocate( {} );
            LUMEN_CHECK_HAS_ERROR( Empty );
            LUMEN_CHECK_EQ( Empty.Error().Code, ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( Empty.Error().Condition, ERHIErrorCondition::Memory );

            const TExpected<FRHIUploadAllocation, FRHIError> Allocation =
                ( *Upload )->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
            LUMEN_CHECK_TRUE( Allocation.HasValue() );
            LUMEN_CHECK_TRUE( ( *Upload )->Free( Allocation->Allocation ).HasValue() );
            const FRHIExpected DoubleFree = ( *Upload )->Free( Allocation->Allocation );
            LUMEN_CHECK_HAS_ERROR( DoubleFree );
            LUMEN_CHECK_EQ( DoubleFree.Error().Condition, ERHIErrorCondition::Memory );

            const TExpected<TSharedRef<IRHIReadbackAllocator>, FRHIError> Readback =
                ( *Device )->CreateReadbackAllocator( 256ULL );
            LUMEN_CHECK_TRUE( Readback.HasValue() );
            const TExpected<FRHIReadbackAllocation, FRHIError> InvalidMap = ( *Readback )->Map( {} );
            LUMEN_CHECK_HAS_ERROR( InvalidMap );
            LUMEN_CHECK_EQ( InvalidMap.Error().Condition, ERHIErrorCondition::Memory );
            const TExpected<FRHIAllocation, FRHIError> ReadbackAllocation =
                ( *Readback )->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
            LUMEN_CHECK_TRUE( ReadbackAllocation.HasValue() );
            const TExpected<FRHIReadbackAllocation, FRHIError> Mapped = ( *Readback )->Map( *ReadbackAllocation );
            LUMEN_CHECK_TRUE( Mapped.HasValue() );
            LUMEN_CHECK_EQ( Mapped->MappedData.size(), 32U );
            LUMEN_CHECK_TRUE( ( *Readback )->Free( *ReadbackAllocation ).HasValue() );
        }

        LUMEN_TEST( RHIMemoryConformance, TransientPlacementNeverAliasesOverlappingLifetimes )
        {
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            const TExpected<TSharedRef<IRHITransientResourceAllocator>, FRHIError> Allocator =
                ( *Device )->CreateTransientResourceAllocator();
            LUMEN_CHECK_TRUE( Allocator.HasValue() );

            FRHITransientBatch DisjointBatch;
            DisjointBatch.Requests.push_back( { .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                                .Lifetime     = { .Begin = 0ULL, .End = 1ULL } } );
            DisjointBatch.Requests.push_back( { .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                                .Lifetime     = { .Begin = 1ULL, .End = 2ULL } } );
            ( *Allocator )->BeginBatch();
            const TExpected<TVector<FRHITransientAllocation>, FRHIError> Disjoint = ( *Allocator )->Allocate( DisjointBatch );
            LUMEN_CHECK_TRUE( Disjoint.HasValue() );
            LUMEN_CHECK_EQ( Disjoint->size(), 2U );
            if ( ( *Disjoint )[0].Allocation.Offset == ( *Disjoint )[1].Allocation.Offset )
            {
                LUMEN_CHECK_FALSE( ( *Disjoint )[0].Lifetime.Overlaps( ( *Disjoint )[1].Lifetime ) );
            }
            ( *Allocator )->EndBatch();

            DisjointBatch.Requests[1].Lifetime = { .Begin = 0ULL, .End = 2ULL };
            ( *Allocator )->BeginBatch();
            const TExpected<TVector<FRHITransientAllocation>, FRHIError> Overlapping = ( *Allocator )->Allocate( DisjointBatch );
            ( *Allocator )->EndBatch();
            LUMEN_CHECK_TRUE( Overlapping.HasValue() );
            LUMEN_CHECK_EQ( Overlapping->size(), 2U );
            LUMEN_CHECK_NOT_EQ( ( *Overlapping )[0].Allocation.Offset, ( *Overlapping )[1].Allocation.Offset );
        }

        LUMEN_TEST( RHIMemoryConformance, ExposesStagingBuffersUsableByValidatedCopies )
        {
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device =
                Tests::GetRHIConformanceDeviceFactory().CreateDevice( true );
            LUMEN_CHECK_TRUE( Device.HasValue() );
            const TExpected<TSharedRef<IRHIUploadAllocator>, FRHIError> Upload = ( *Device )->CreateUploadAllocator( 256ULL );
            const TExpected<TSharedRef<IRHIReadbackAllocator>, FRHIError> Readback =
                ( *Device )->CreateReadbackAllocator( 256ULL );
            LUMEN_CHECK_TRUE( Upload.HasValue() );
            LUMEN_CHECK_TRUE( Readback.HasValue() );

            TExpected<FRHIUploadAllocation, FRHIError> UploadAllocation =
                ( *Upload )->Allocate( { .Size = 64ULL, .Alignment = 16ULL } );
            TExpected<FRHIAllocation, FRHIError> ReadbackAllocation =
                ( *Readback )->Allocate( { .Size = 64ULL, .Alignment = 16ULL } );
            LUMEN_CHECK_TRUE( UploadAllocation.HasValue() );
            LUMEN_CHECK_TRUE( ReadbackAllocation.HasValue() );
            LUMEN_CHECK_EQ( UploadAllocation->Buffer->GetDesc().MemoryClass, ERHIMemoryClass::Upload );
            LUMEN_CHECK_TRUE( HasAnyFlags( UploadAllocation->Buffer->GetDesc().Usage, ERHIResourceUsage::CopySource ) );

            UploadAllocation->MappedData[0] = Byte{ 0xA5U };
            TExpected<TSpan<Byte>, FRHIError> SharedBytes =
                UploadAllocation->Buffer->Map( UploadAllocation->Allocation.Offset, UploadAllocation->Allocation.Size );
            LUMEN_CHECK_TRUE( SharedBytes.HasValue() );
            LUMEN_CHECK_EQ( static_cast<UInt32>( ( *SharedBytes )[0] ), 0xA5U );
            UploadAllocation->Buffer->Unmap();

            TExpected<TSharedRef<IRHIBuffer>, FRHIError> ReadbackBuffer = ( *Readback )->GetBuffer( *ReadbackAllocation );
            LUMEN_CHECK_TRUE( ReadbackBuffer.HasValue() );
            LUMEN_CHECK_EQ( ( *ReadbackBuffer )->GetDesc().MemoryClass, ERHIMemoryClass::Readback );
            LUMEN_CHECK_TRUE( HasAnyFlags( ( *ReadbackBuffer )->GetDesc().Usage, ERHIResourceUsage::CopyDestination ) );
            TExpected<TSharedRef<IRHIBuffer>, FRHIError> SameBuffer = ( *Readback )->GetBuffer( *ReadbackAllocation );
            LUMEN_CHECK_TRUE( SameBuffer.HasValue() );
            LUMEN_CHECK_EQ( ( *ReadbackBuffer )->GetId().GetValue(), ( *SameBuffer )->GetId().GetValue() );

            TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
            const FRHIBufferCopy Copy{ .SourceOffset      = UploadAllocation->Allocation.Offset,
                                       .DestinationOffset = ReadbackAllocation->Offset,
                                       .Size              = UploadAllocation->Allocation.Size };
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            LUMEN_CHECK_TRUE( List->CopyBuffer( UploadAllocation->Buffer, *ReadbackBuffer, Copy ).HasValue() );
            LUMEN_CHECK_TRUE( List->End().HasValue() );
            LUMEN_CHECK_TRUE( ( *Upload )->Free( UploadAllocation->Allocation ).HasValue() );
            LUMEN_CHECK_TRUE( ( *Readback )->Free( *ReadbackAllocation ).HasValue() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
