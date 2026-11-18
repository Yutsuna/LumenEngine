/**
 * @file RHIPoolAllocatorTests.cpp
 * @brief Tests fixed-size pool allocation.
 */

#include <Memory/RHIPoolAllocator.hpp>
#include <Tests/UnitTest.hpp>
#include <Types/RHIError.hpp>

namespace Lumen
{

LUMEN_TEST( RHIPoolAllocator, AllocatesRecyclesAndRejectsWrongClass )
{
    FRHIPoolAllocator Allocator( 64ULL, 2U );
    const TExpected<FRHIAllocation, FRHIError> First  = Allocator.Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
    const TExpected<FRHIAllocation, FRHIError> Second = Allocator.Allocate( { .Size = 64ULL, .Alignment = 64ULL } );

    LUMEN_CHECK_TRUE( First.HasValue() and Second.HasValue() );
    LUMEN_CHECK_HAS_ERROR( Allocator.Allocate( { 1ULL, 1ULL } ) );
    LUMEN_CHECK_HAS_ERROR( Allocator.Allocate( { 65ULL, 1ULL } ) );
    LUMEN_CHECK_TRUE( Allocator.Free( *First ).HasValue() );
    LUMEN_CHECK_TRUE( Allocator.Allocate( { 64ULL, 1ULL } ).HasValue() );
}

LUMEN_TEST( RHIPoolAllocator, RejectsNonPowerOfTwoSlotSize )
{
    FRHIPoolAllocator Allocator( 24ULL, 4U );
    LUMEN_CHECK_EQ( Allocator.GetStats().Capacity, 0ULL );

    const TExpected<FRHIAllocation, FRHIError> Result = Allocator.Allocate( { .Size = 8ULL, .Alignment = 8ULL } );
    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, ERHIErrorCode::InvalidState );

    LUMEN_CHECK_HAS_ERROR( Allocator.Allocate( { .Size = 24ULL, .Alignment = 16ULL } ) );
    LUMEN_CHECK_EQ( Allocator.GetStats().Used, 0ULL );
}

LUMEN_TEST( RHIPoolAllocator, RejectsCapacityOverflow )
{
    constexpr UInt64 LargeSlotSize = 1ULL << 63;
    FRHIPoolAllocator Allocator( LargeSlotSize, 3U );
    LUMEN_CHECK_EQ( Allocator.GetStats().Capacity, 0ULL );

    const TExpected<FRHIAllocation, FRHIError> Result = Allocator.Allocate( { .Size = 1ULL, .Alignment = 1ULL } );
    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, ERHIErrorCode::InvalidState );

    LUMEN_CHECK_EQ( Allocator.GetStats().Used, 0ULL );
}

LUMEN_TEST( RHIPoolAllocator, HonorsAlignmentForValidPowerOfTwoSlots )
{
    FRHIPoolAllocator Allocator( 64ULL, 2U );
    LUMEN_CHECK_EQ( Allocator.GetStats().Capacity, 128ULL );

    const TExpected<FRHIAllocation, FRHIError> First  = Allocator.Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
    const TExpected<FRHIAllocation, FRHIError> Second = Allocator.Allocate( { .Size = 64ULL, .Alignment = 64ULL } );
    LUMEN_CHECK_TRUE( First.HasValue() and Second.HasValue() );

    LUMEN_CHECK_EQ( First->Offset, 0ULL );
    LUMEN_CHECK_EQ( Second->Offset, 64ULL );
    LUMEN_CHECK_EQ( First->Offset % 16ULL, 0ULL );
    LUMEN_CHECK_EQ( Second->Offset % 64ULL, 0ULL );
    LUMEN_CHECK_EQ( First->Alignment, 64ULL );
    LUMEN_CHECK_EQ( Second->Alignment, 64ULL );
}

} // namespace Lumen
