/**
 * @file RHIDefragPlanTests.cpp
 * @brief Tests defragmentation plan validation.
 */

#include <RHI/Memory/RHIDefragPlan.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIDefragPlan, RejectsOverlappingDestinations )
    {
        LUMEN_CHECK_TRUE( FRHIDefragPlan{}.Validate().HasValue() );

        const FRHIAllocation Source{ .Id = FRHIAllocationId{ 1ULL }, .Offset = 0ULL, .Size = 16ULL };
        const FRHIAllocation Destination{ .Id = FRHIAllocationId{ 2ULL }, .Offset = 32ULL, .Size = 16ULL };

        FRHIDefragPlan Plan;

        Plan.Moves = { { Source, Destination }, { Source, Destination } };
        LUMEN_CHECK_HAS_ERROR( Plan.Validate() );
    }

    LUMEN_TEST( RHIDefragPlan, AcceptsOverlappingOffsetsInDifferentHeaps )
    {
        UInt64 FirstHeapToken  = 0ULL;
        UInt64 SecondHeapToken = 0ULL;
        IRHIHeap *FirstHeap    = reinterpret_cast<IRHIHeap *>( &FirstHeapToken );
        IRHIHeap *SecondHeap   = reinterpret_cast<IRHIHeap *>( &SecondHeapToken );

        const FRHIAllocation FirstSource{ .Id = FRHIAllocationId{ 1ULL }, .Offset = 2048ULL, .Size = 16ULL, .Heap = FirstHeap };
        const FRHIAllocation FirstDestination{ .Id = FRHIAllocationId{ 2ULL }, .Offset = 0ULL, .Size = 16ULL, .Heap = FirstHeap };
        const FRHIAllocation SecondSource{ .Id = FRHIAllocationId{ 3ULL }, .Offset = 2048ULL, .Size = 16ULL, .Heap = SecondHeap };
        const FRHIAllocation SecondDestination{
            .Id = FRHIAllocationId{ 4ULL }, .Offset = 0ULL, .Size = 16ULL, .Heap = SecondHeap };

        FRHIDefragPlan Plan;
        Plan.Moves = { { FirstSource, FirstDestination }, { SecondSource, SecondDestination } };
        LUMEN_CHECK_TRUE( Plan.Validate().HasValue() );
    }

    LUMEN_TEST( RHIDefragPlan, RejectsOverlappingDestinationsWithinSameHeap )
    {
        UInt64 HeapToken = 0ULL;
        IRHIHeap *Heap   = reinterpret_cast<IRHIHeap *>( &HeapToken );

        const FRHIAllocation Source{ .Id = FRHIAllocationId{ 1ULL }, .Offset = 2048ULL, .Size = 16ULL, .Heap = Heap };
        const FRHIAllocation FirstDestination{ .Id = FRHIAllocationId{ 2ULL }, .Offset = 0ULL, .Size = 16ULL, .Heap = Heap };
        const FRHIAllocation SecondDestination{ .Id = FRHIAllocationId{ 3ULL }, .Offset = 0ULL, .Size = 16ULL, .Heap = Heap };

        FRHIDefragPlan Plan;
        Plan.Moves = { { Source, FirstDestination }, { Source, SecondDestination } };
        LUMEN_CHECK_HAS_ERROR( Plan.Validate() );
    }

} // namespace RHI

} // namespace Lumen
