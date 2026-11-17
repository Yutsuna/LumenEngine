/**
 * @file RHIReadbackAllocatorTests.cpp
 * @brief Tests readback allocation and mapping.
 */

#include "Device/NullDevice.hpp"
#include <Memory/IRHIReadbackAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
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

} // namespace Lumen
