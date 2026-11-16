/**
 * @file RHIRingAllocatorTests.cpp
 * @brief Tests circular ranges and retirement.
 */

#include <Memory/RHIRingAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIRingAllocator, ReusesOnlyReleasedRanges )
{
    FRHIRingAllocator Allocator( 128ULL );
    auto First  = Allocator.Allocate( { .Size = 64ULL, .Alignment = 16ULL } );
    auto Second = Allocator.Allocate( { .Size = 64ULL, .Alignment = 16ULL } );

    LUMEN_CHECK_TRUE( Second.HasValue() );
    LUMEN_CHECK_HAS_ERROR( Allocator.Allocate( { 96ULL, 1ULL } ) );
    LUMEN_CHECK_TRUE( Allocator.Free( *First ).HasValue() );
    LUMEN_CHECK_HAS_ERROR( Allocator.Free( *First ) );

    auto Wrapped = Allocator.Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
    LUMEN_CHECK_TRUE( Wrapped.HasValue() );
    LUMEN_CHECK_EQ( Wrapped->Offset, 0ULL );
}

} // namespace Lumen
