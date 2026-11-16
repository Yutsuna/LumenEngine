/**
 * @file RHITlsfAllocatorTests.cpp
 * @brief Tests segregated allocation and coalescing.
 */

#include <Memory/RHITlsfAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHITlsfAllocator, CoalescesAdjacentReleasedRanges )
{
    FRHITlsfAllocator Allocator( 256ULL );
    auto First  = Allocator.Allocate( { .Size = 64ULL, .Alignment = 16ULL } );
    auto Second = Allocator.Allocate( { .Size = 64ULL, .Alignment = 16ULL } );

    LUMEN_CHECK_EQ( Second->Offset, 64ULL );
    LUMEN_CHECK_TRUE( Allocator.Free( *First ).HasValue() );
    LUMEN_CHECK_TRUE( Allocator.Free( *Second ).HasValue() );
    LUMEN_CHECK_HAS_ERROR( Allocator.Free( *Second ) );

    auto Joined = Allocator.Allocate( { .Size = 256ULL, .Alignment = 16ULL } );
    LUMEN_CHECK_TRUE( Joined.HasValue() );
    LUMEN_CHECK_EQ( Joined->Offset, 0ULL );
}

} // namespace Lumen
