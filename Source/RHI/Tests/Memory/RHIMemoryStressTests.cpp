/**
 * @file RHIMemoryStressTests.cpp
 * @brief Stresses repeated variable allocations.
 */

#include <Memory/RHITlsfAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIMemoryStress, RepeatedAllocationAndCoalescingStayBalanced )
{
    FRHITlsfAllocator Allocator( 65536ULL );

    for ( UInt32 Index = 0U; Index < 512U; ++Index )
    {
        auto Allocation = Allocator.Allocate( { .Size = 64ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_TRUE( Allocation.HasValue() );
        LUMEN_CHECK_TRUE( Allocator.Free( *Allocation ).HasValue() );
    }

    LUMEN_CHECK_EQ( Allocator.GetStats().Used, 0ULL );
}

} // namespace Lumen
