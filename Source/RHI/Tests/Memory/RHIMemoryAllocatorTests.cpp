/**
 * @file RHIMemoryAllocatorTests.cpp
 * @brief Tests budgeted allocation composition.
 */

#include <RHI/Memory/RHILinearAllocator.hpp>
#include <RHI/Memory/RHIMemoryAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIMemoryAllocator, ReleasesBudgetOnSuccessAndFailure )
    {
        FRHILinearAllocator Allocator( 128ULL );
        FRHIMemoryBudget Budget( 64ULL );
        FRHIMemoryAllocator Facade( Allocator, Budget );
        const TExpected<FRHIAllocation, FRHIError> Allocation = Facade.Allocate( { 32ULL, 16ULL } );
        LUMEN_CHECK_TRUE( Allocation.HasValue() );
        LUMEN_CHECK_EQ( Budget.GetUsed(), 32ULL );
        LUMEN_CHECK_HAS_ERROR( Facade.Allocate( { 64ULL, 1ULL } ) );
        LUMEN_CHECK_EQ( Budget.GetUsed(), 32ULL );
        LUMEN_CHECK_TRUE( Facade.Free( *Allocation ).HasValue() );
        LUMEN_CHECK_EQ( Budget.GetUsed(), 0ULL );
    }

} // namespace RHI

} // namespace Lumen
