/**
 * @file RHIDefragmenter.cpp
 * @brief Implements conservative defragmentation planning.
 */

#include "RHIDefragmenter.hpp"
#include <Algo/Sort.hpp>
#include <RHI/Memory/IRHIAllocationObserver.hpp>
#include <RHI/Types/RHITimeline.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>
#include <Templates/Hash.hpp>

Lumen::RHI::FRHIDefragmenter::FRHIDefragmenter ( const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FRHIDefragmenter>() )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( FRHIDefragmenter )
Lumen::TExpected<Lumen::RHI::FRHIDefragPlan, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIDefragmenter::BuildPlan ( const TSpan<const FRHIAllocation> InAllocations ) noexcept
{
    TVector<FRHIAllocation> Sorted;
    Sorted.reserve( InAllocations.size() );
    for ( const FRHIAllocation &Allocation : InAllocations )
    {
        if ( not Allocation.IsValid() )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Invalid live allocation" ) );
        }
        Sorted.push_back( Allocation );
    }
    if ( Sorted.empty() )
    {
        return FRHIDefragPlan{};
    }
    Algo::StableSort( Sorted, [] ( const FRHIAllocation &InLeft, const FRHIAllocation &InRight ) noexcept
    {
        return InLeft.Offset < InRight.Offset;
    } );

    Bool bHasPrevious  = false;
    UInt64 PreviousEnd = 0ULL;
    for ( const FRHIAllocation &Allocation : Sorted )
    {
        const TOptional<UInt64> End = RHICheckedAdd( Allocation.Offset, Allocation.Size );
        if ( not End )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Live allocation range overflows" ) );
        }
        if ( bHasPrevious and Allocation.Offset < PreviousEnd )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Overlapping live allocations" ) );
        }
        PreviousEnd  = *End;
        bHasPrevious = true;
    }

    FRHIDefragPlan Plan;
    UInt64 Cursor = Sorted.front().Offset;
    for ( const FRHIAllocation &Allocation : Sorted )
    {
        const TOptional<UInt64> AlignedOffset = RHIAlignUp( Cursor, Allocation.Alignment );
        if ( not AlignedOffset )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Invalid defragmentation alignment" ) );
        }
        const TOptional<UInt64> End = RHICheckedAdd( *AlignedOffset, Allocation.Size );
        if ( not End )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Defragmentation range overflows" ) );
        }
        FRHIAllocation Destination = Allocation;
        Destination.Offset         = *AlignedOffset;
        if ( Destination.Offset != Allocation.Offset )
        {
            Plan.Moves.push_back( FRHIDefragMove{ Allocation, Destination, FRHITimelineValue{ 1ULL } } );
        }
        Cursor = *End;
    }
    const TOptional<UInt64> LastEnd = RHICheckedAdd( Sorted.back().Offset, Sorted.back().Size );
    if ( LastEnd and Cursor <= *LastEnd )
    {
        Plan.ReclaimedBytes = *LastEnd - Cursor;
    }
    return Plan;
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIDefragmenter::Execute ( const FRHIDefragPlan &InPlan ) noexcept
{
    LUMEN_EXPECT( _, InPlan.Validate() );
    for ( const FRHIDefragMove &Move : InPlan.Moves )
    {
        if ( not IsRHICompletionReached( InPlan.ObservedCompletion, Move.Completion ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidState( ERHIErrorCondition::Memory, "Defragmentation source is still in use by the GPU" ) );
        }
    }
    for ( const FRHIDefragMove &Move : InPlan.Moves )
    {
        GetRHIAllocationObserverRegistry().NotifyMoved( Move.Source, Move.Destination );
    }
    return {};
}
