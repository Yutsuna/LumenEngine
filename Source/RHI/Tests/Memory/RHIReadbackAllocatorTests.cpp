/**
 * @file RHIReadbackAllocatorTests.cpp
 * @brief Tests readback allocation and mapping.
 */

#include "Device/NullDevice.hpp"
#include <RHI/Memory/IRHIReadbackAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIReadbackAllocator, MapsAllocatedRangeAndRecycles )
    {
        Tests::FNullDevice Device;
        const TSharedRef<IRHIReadbackAllocator> Allocator         = *Device.CreateReadbackAllocator( 256ULL );
        const TExpected<FRHIAllocation, FRHIError> Allocation     = Allocator->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        const TExpected<FRHIReadbackAllocation, FRHIError> Mapped = Allocator->Map( *Allocation );

        LUMEN_CHECK_TRUE( Mapped.HasValue() );
        LUMEN_CHECK_EQ( Mapped->MappedData.size(), 32U );
        LUMEN_CHECK_TRUE( Allocator->Free( *Allocation ).HasValue() );
    }

    LUMEN_TEST( RHIReadbackAllocator, RejectsForeignMemoryClass )
    {
        Tests::FNullDevice Device;
        const TSharedRef<IRHIReadbackAllocator> Allocator     = *Device.CreateReadbackAllocator( 256ULL );
        const TExpected<FRHIAllocation, FRHIError> Allocation = Allocator->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_HAS_VALUE( Allocation );

        FRHIAllocation DeviceLocalAllocation = *Allocation;
        DeviceLocalAllocation.MemoryClass    = ERHIMemoryClass::DeviceLocal;
        LUMEN_CHECK_HAS_ERROR( Allocator->Map( DeviceLocalAllocation ) );

        /** NOTE: Upload is CPU accessible, but a readback allocator must still refuse it. */
        FRHIAllocation UploadAllocation = *Allocation;
        UploadAllocation.MemoryClass    = ERHIMemoryClass::Upload;
        LUMEN_CHECK_HAS_ERROR( Allocator->Map( UploadAllocation ) );
    }

    LUMEN_TEST( RHIReadbackAllocator, RejectsAllocationFromAnotherAllocator )
    {
        Tests::FNullDevice Device;
        const TSharedRef<IRHIReadbackAllocator> LocalAllocator   = *Device.CreateReadbackAllocator( 256ULL );
        const TSharedRef<IRHIReadbackAllocator> ForeignAllocator = *Device.CreateReadbackAllocator( 256ULL );

        const TExpected<FRHIAllocation, FRHIError> LocalFirst = LocalAllocator->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        const TExpected<FRHIAllocation, FRHIError> LocalSecond =
            LocalAllocator->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_HAS_VALUE( LocalFirst );
        LUMEN_CHECK_HAS_VALUE( LocalSecond );
        LUMEN_CHECK_HAS_VALUE( LocalAllocator->Free( *LocalFirst ) );

        const TExpected<FRHIAllocation, FRHIError> Foreign = ForeignAllocator->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_HAS_VALUE( Foreign );
        LUMEN_CHECK_HAS_ERROR( LocalAllocator->Map( *Foreign ) );
    }

    LUMEN_TEST( RHIReadbackAllocator, RejectsFreedAllocation )
    {
        Tests::FNullDevice Device;
        const TSharedRef<IRHIReadbackAllocator> Allocator     = *Device.CreateReadbackAllocator( 256ULL );
        const TExpected<FRHIAllocation, FRHIError> Allocation = Allocator->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_HAS_VALUE( Allocation );
        LUMEN_CHECK_HAS_VALUE( Allocator->Free( *Allocation ) );
        LUMEN_CHECK_HAS_ERROR( Allocator->Map( *Allocation ) );
    }

} // namespace RHI

} // namespace Lumen
