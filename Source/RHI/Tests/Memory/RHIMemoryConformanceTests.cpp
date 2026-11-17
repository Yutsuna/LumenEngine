/**
 * @file RHIMemoryConformanceTests.cpp
 * @brief Runs portable heap, staging, and transient allocation contracts.
 */

#include <Device/IRHIDevice.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>

#include <Memory/IRHIReadbackAllocator.hpp>
#include <Memory/IRHITransientResourceAllocator.hpp>
#include <Memory/IRHIUploadAllocator.hpp>
#include <Memory/RHIHeapDesc.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIMemoryConformance, CreatesHeapsAndStagingAllocators )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        LUMEN_CHECK_TRUE( ( *Device )->CreateHeap( { .Size = 4096ULL, .MemoryClass = ERHIMemoryClass::Upload } ).HasValue() );

        auto Upload = ( *Device )->CreateUploadAllocator( 4096ULL );
        LUMEN_CHECK_TRUE( Upload.HasValue() );

        auto Allocation = ( *Upload )->Allocate( { .Size = 64ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_TRUE( Allocation.HasValue() );
        LUMEN_CHECK_EQ( Allocation->MappedData.size(), 64U );
        LUMEN_CHECK_TRUE( ( *Upload )->Free( Allocation->Allocation ).HasValue() );
    }

    LUMEN_TEST( RHIMemoryConformance, RejectsInvalidStagingOperationsAndDoubleFree )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        auto Upload = ( *Device )->CreateUploadAllocator( 256ULL );
        LUMEN_CHECK_TRUE( Upload.HasValue() );
        const auto Empty = ( *Upload )->Allocate( {} );
        LUMEN_CHECK_HAS_ERROR( Empty );
        LUMEN_CHECK_EQ( Empty.Error().Code, ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( Empty.Error().Condition, ERHIErrorCondition::Memory );

        auto Allocation = ( *Upload )->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_TRUE( Allocation.HasValue() );
        LUMEN_CHECK_TRUE( ( *Upload )->Free( Allocation->Allocation ).HasValue() );
        const auto DoubleFree = ( *Upload )->Free( Allocation->Allocation );
        LUMEN_CHECK_HAS_ERROR( DoubleFree );
        LUMEN_CHECK_EQ( DoubleFree.Error().Condition, ERHIErrorCondition::Memory );

        auto Readback = ( *Device )->CreateReadbackAllocator( 256ULL );
        LUMEN_CHECK_TRUE( Readback.HasValue() );
        const auto InvalidMap = ( *Readback )->Map( {} );
        LUMEN_CHECK_HAS_ERROR( InvalidMap );
        LUMEN_CHECK_EQ( InvalidMap.Error().Condition, ERHIErrorCondition::Memory );
        auto ReadbackAllocation = ( *Readback )->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_TRUE( ReadbackAllocation.HasValue() );
        auto Mapped = ( *Readback )->Map( *ReadbackAllocation );
        LUMEN_CHECK_TRUE( Mapped.HasValue() );
        LUMEN_CHECK_EQ( Mapped->MappedData.size(), 32U );
        LUMEN_CHECK_TRUE( ( *Readback )->Free( *ReadbackAllocation ).HasValue() );
    }

    LUMEN_TEST( RHIMemoryConformance, TransientPlacementNeverAliasesOverlappingLifetimes )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        auto Allocator = ( *Device )->CreateTransientResourceAllocator();
        LUMEN_CHECK_TRUE( Allocator.HasValue() );

        FRHITransientBatch DisjointBatch;
        DisjointBatch.Requests.push_back( { .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                            .Lifetime     = { .Begin = 0ULL, .End = 1ULL } } );
        DisjointBatch.Requests.push_back( { .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                            .Lifetime     = { .Begin = 1ULL, .End = 2ULL } } );
        ( *Allocator )->BeginBatch();
        auto Disjoint = ( *Allocator )->Allocate( DisjointBatch );
        LUMEN_CHECK_TRUE( Disjoint.HasValue() );
        LUMEN_CHECK_EQ( Disjoint->size(), 2U );
        if ( ( *Disjoint )[0].Allocation.Offset == ( *Disjoint )[1].Allocation.Offset )
        {
            LUMEN_CHECK_FALSE( ( *Disjoint )[0].Lifetime.Overlaps( ( *Disjoint )[1].Lifetime ) );
        }
        ( *Allocator )->EndBatch();

        FRHITransientBatch OverlappingBatch   = DisjointBatch;
        OverlappingBatch.Requests[1].Lifetime = { .Begin = 0ULL, .End = 2ULL };
        ( *Allocator )->BeginBatch();
        auto Overlapping = ( *Allocator )->Allocate( OverlappingBatch );
        ( *Allocator )->EndBatch();
        LUMEN_CHECK_TRUE( Overlapping.HasValue() );
        LUMEN_CHECK_EQ( Overlapping->size(), 2U );
        LUMEN_CHECK_NOT_EQ( ( *Overlapping )[0].Allocation.Offset, ( *Overlapping )[1].Allocation.Offset );
    }

} // namespace
} // namespace Lumen
