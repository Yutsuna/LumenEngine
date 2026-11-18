/**
 * @file RHIDefragPlanTests.cpp
 * @brief Tests defragmentation plan validation.
 */

#include <Memory/RHIDefragPlan.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
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

} // namespace Lumen
