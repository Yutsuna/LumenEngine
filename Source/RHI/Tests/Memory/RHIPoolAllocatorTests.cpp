/**
 * @file RHIPoolAllocatorTests.cpp
 * @brief Tests fixed-size pool allocation.
 */

#include <Memory/RHIPoolAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIPoolAllocator, AllocatesRecyclesAndRejectsWrongClass )
{
    FRHIPoolAllocator Allocator( 64ULL, 2U );
    auto First  = Allocator.Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
    auto Second = Allocator.Allocate( { .Size = 64ULL, .Alignment = 64ULL } );

    LUMEN_CHECK_TRUE( First.HasValue() and Second.HasValue() );
    LUMEN_CHECK_HAS_ERROR( Allocator.Allocate( { 1ULL, 1ULL } ) );
    LUMEN_CHECK_HAS_ERROR( Allocator.Allocate( { 65ULL, 1ULL } ) );
    LUMEN_CHECK_TRUE( Allocator.Free( *First ).HasValue() );
    LUMEN_CHECK_TRUE( Allocator.Allocate( { 64ULL, 1ULL } ).HasValue() );
}

} // namespace Lumen
