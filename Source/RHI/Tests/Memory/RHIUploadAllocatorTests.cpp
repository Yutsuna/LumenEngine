/**
 * @file RHIUploadAllocatorTests.cpp
 * @brief Tests writable staging ranges.
 */

#include <Device/NullDevice.hpp>
#include <RHI/Memory/IRHIUploadAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIUploadAllocator, ProvidesWritableAlignedStorage )
    {
        Tests::FNullDevice Device;
        const TSharedRef<IRHIUploadAllocator> Allocator       = *Device.CreateUploadAllocator( 256ULL );
        TExpected<FRHIUploadAllocation, FRHIError> Allocation = Allocator->Allocate( { .Size = 32ULL, .Alignment = 16ULL } );

        LUMEN_CHECK_TRUE( Allocation.HasValue() );
        Allocation->MappedData[0] = 0xABU;
        LUMEN_CHECK_EQ( Allocation->MappedData[0], 0xABU );
        LUMEN_CHECK_TRUE( Allocator->Free( Allocation->Allocation ).HasValue() );
    }

} // namespace RHI

} // namespace Lumen
