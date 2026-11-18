/**
 * @file RHIDefragmenterTests.cpp
 * @brief Tests conservative defragmentation.
 */

#include <Device/NullDevice.hpp>
#include <Memory/IRHIDefragmenter.hpp>
#include <Tests/UnitTest.hpp>
#include <Types/RHITimeline.hpp>
#include <Utilities/RHIAlignment.hpp>

namespace Lumen
{

LUMEN_TEST( RHIDefragmenter, EmptyPlanIsValidAndExecutable )
{
    Tests::FNullDevice Device;
    const TSharedRef<IRHIDefragmenter> Defragmenter = *Device.CreateDefragmenter();
    const TExpected<FRHIDefragPlan, FRHIError> Plan = Defragmenter->BuildPlan( {} );
    LUMEN_CHECK_TRUE( Plan.HasValue() );
    LUMEN_CHECK_TRUE( Defragmenter->Execute( *Plan ).HasValue() );
}

LUMEN_TEST( RHIDefragmenter, GeneratesNonOverlappingMovesWithCompletionGate )
{
    Tests::FNullDevice Device;
    const TSharedRef<IRHIDefragmenter> Defragmenter = *Device.CreateDefragmenter();

    const FRHIAllocation First{ .Id = FRHIAllocationId{ 1ULL }, .Offset = 0ULL, .Size = 64ULL, .Alignment = 16ULL };
    const FRHIAllocation Second{ .Id = FRHIAllocationId{ 2ULL }, .Offset = 128ULL, .Size = 64ULL, .Alignment = 16ULL };
    const FRHIAllocation Third{ .Id = FRHIAllocationId{ 3ULL }, .Offset = 256ULL, .Size = 32ULL, .Alignment = 32ULL };
    const FRHIAllocation LiveAllocations[] = { First, Second, Third };

    const TExpected<FRHIDefragPlan, FRHIError> PlanResult = Defragmenter->BuildPlan( LiveAllocations );
    LUMEN_CHECK_TRUE( PlanResult.HasValue() );
    const FRHIDefragPlan &Plan = *PlanResult;
    LUMEN_CHECK_FALSE( Plan.Moves.empty() );
    LUMEN_CHECK_TRUE( Plan.Validate().HasValue() );
    LUMEN_CHECK_TRUE( Plan.ReclaimedBytes > 0ULL );

    for ( USize FirstIndex = 0U; FirstIndex < Plan.Moves.size(); ++FirstIndex )
    {
        const FRHIDefragMove &Left = Plan.Moves[FirstIndex];
        LUMEN_CHECK_EQ( Left.Source.Size, Left.Destination.Size );
        for ( USize SecondIndex = FirstIndex + 1U; SecondIndex < Plan.Moves.size(); ++SecondIndex )
        {
            const FRHIDefragMove &Right = Plan.Moves[SecondIndex];
            LUMEN_CHECK_FALSE( RHIRangesOverlap( Left.Destination.Offset, Left.Destination.Size, Right.Destination.Offset,
                                                 Right.Destination.Size ) );
        }
    }

    LUMEN_CHECK_HAS_ERROR( Defragmenter->Execute( Plan ) );
    FRHIDefragPlan ReadyPlan     = Plan;
    ReadyPlan.ObservedCompletion = FRHITimelineValue{ 1ULL };
    LUMEN_CHECK_TRUE( Defragmenter->Execute( ReadyPlan ).HasValue() );
}

LUMEN_TEST( RHIDefragmenter, RejectsOverlappingLiveAllocations )
{
    Tests::FNullDevice Device;
    const TSharedRef<IRHIDefragmenter> Defragmenter = *Device.CreateDefragmenter();

    const FRHIAllocation First{ .Id = FRHIAllocationId{ 1ULL }, .Offset = 0ULL, .Size = 64ULL };
    const FRHIAllocation Second{ .Id = FRHIAllocationId{ 2ULL }, .Offset = 32ULL, .Size = 64ULL };
    const FRHIAllocation LiveAllocations[] = { First, Second };

    LUMEN_CHECK_HAS_ERROR( Defragmenter->BuildPlan( LiveAllocations ) );
}

} // namespace Lumen
