/**
 * @file RHIRingAllocatorTests.cpp
 * @brief Tests circular ranges and retirement.
 */

#include <RHI/Memory/RHIRingAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIRingAllocator, ReusesOnlyReleasedRanges )
    {
        FRHIRingAllocator Allocator( 128ULL );
        const TExpected<FRHIAllocation, FRHIError> First  = Allocator.Allocate( { .Size = 64ULL, .Alignment = 16ULL } );
        const TExpected<FRHIAllocation, FRHIError> Second = Allocator.Allocate( { .Size = 64ULL, .Alignment = 16ULL } );

        LUMEN_CHECK_TRUE( Second.HasValue() );
        LUMEN_CHECK_HAS_ERROR( Allocator.Allocate( { 96ULL, 1ULL } ) );
        LUMEN_CHECK_TRUE( Allocator.Free( *First ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( Allocator.Free( *First ) );

        const TExpected<FRHIAllocation, FRHIError> Wrapped = Allocator.Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_TRUE( Wrapped.HasValue() );
        LUMEN_CHECK_EQ( Wrapped->Offset, 0ULL );
    }

} // namespace RHI

} // namespace Lumen
