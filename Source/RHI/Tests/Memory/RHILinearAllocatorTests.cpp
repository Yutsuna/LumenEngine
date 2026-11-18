/**
 * @file RHILinearAllocatorTests.cpp
 * @brief Tests bump allocation and reset.
 */

#include <Memory/RHILinearAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHILinearAllocator, AlignsExhaustsAndResets )
{
    FRHILinearAllocator Allocator( 128ULL );
    const TExpected<FRHIAllocation, FRHIError> First  = Allocator.Allocate( { 8ULL, 16ULL } );
    const TExpected<FRHIAllocation, FRHIError> Second = Allocator.Allocate( { 8ULL, 16ULL } );
    LUMEN_CHECK_EQ( First->Offset, 0ULL );
    LUMEN_CHECK_EQ( Second->Offset, 16ULL );
    LUMEN_CHECK_HAS_ERROR( Allocator.Allocate( { 256ULL, 1ULL } ) );
    Allocator.Reset();
    LUMEN_CHECK_EQ( Allocator.GetStats().Used, 0ULL );
    LUMEN_CHECK_TRUE( Allocator.Allocate( { 128ULL, 1ULL } ).HasValue() );
}

} // namespace Lumen
