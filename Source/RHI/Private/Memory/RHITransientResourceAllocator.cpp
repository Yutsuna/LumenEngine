/**
 * @file RHITransientResourceAllocator.cpp
 * @brief Implements transient batches and disjoint-lifetime aliasing.
 */

#include "RHITransientResourceAllocator.hpp"
#include <Templates/Hash.hpp>

namespace
{

/** @return True when an already placed range may be reused for the next request. */
Lumen::Bool CanAliasTransientRange ( const Lumen::FRHIResourceRequirements &InPlaced,
                                     const Lumen::FRHIResourceRequirements &InNext ) noexcept
{
    const Lumen::TOptional<Lumen::ERHIMemoryClass> &PlacedClass = InPlaced.PreferredMemoryClass;
    const Lumen::TOptional<Lumen::ERHIMemoryClass> &NextClass   = InNext.PreferredMemoryClass;
    if ( PlacedClass.HasValue() != NextClass.HasValue() )
    {
        return false;
    }
    if ( PlacedClass.HasValue() and *PlacedClass != *NextClass )
    {
        return false;
    }
    /** NOTE: Sizes and alignments are the validated values from ToAllocationDesc(). */
    return InNext.Size <= InPlaced.Size and InNext.Alignment <= InPlaced.Alignment;
}

} // namespace

Lumen::FRHITransientResourceAllocator::FRHITransientResourceAllocator ( const FRHIObjectId InId,
                                                                        const UInt64 InCapacity ) noexcept
    : Identity( InId, FHash::TypeHash<FRHITransientResourceAllocator>() ), Allocator( InCapacity )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( FRHITransientResourceAllocator )
void Lumen::FRHITransientResourceAllocator::BeginBatch () noexcept
{
    bInBatch = true;
}

Lumen::TExpected<Lumen::TVector<Lumen::FRHITransientAllocation>, Lumen::FRHIError>
Lumen::FRHITransientResourceAllocator::Allocate ( const FRHITransientBatch &InBatch ) noexcept
{
    if ( not bInBatch )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidState( ERHIErrorCondition::Memory, "Transient batch is not active" ) );
    }
    if ( const TExpected<void, FRHIError> Valid = InBatch.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    TVector<FRHITransientAllocation> Results;
    TVector<FRHIResourceRequirements> PlacedRequirements;
    TVector<Bool> Released;
    for ( USize RequestIndex = 0U; RequestIndex < InBatch.Requests.size(); ++RequestIndex )
    {
        const FRHITransientDesc &Request = InBatch.Requests[RequestIndex];
        for ( USize Index = 0U; Index < Results.size(); ++Index )
        {
            Bool bOverlapsRemainingRequest = Released[Index];
            for ( USize FutureIndex = RequestIndex; FutureIndex < InBatch.Requests.size(); ++FutureIndex )
            {
                bOverlapsRemainingRequest =
                    bOverlapsRemainingRequest or Results[Index].Lifetime.Overlaps( InBatch.Requests[FutureIndex].Lifetime );
            }
            const Bool bCanAlias =
                not bOverlapsRemainingRequest and CanAliasTransientRange( PlacedRequirements[Index], Request.Requirements );
            if ( bCanAlias )
            {
                const TExpected<void, FRHIError> Freed = Allocator.Free( Results[Index].Allocation );
                if ( not Freed )
                {
                    return TUnexpected<FRHIError>( Freed.Error() );
                }
                Released[Index] = true;
            }
        }
        const TExpected<FRHIAllocation, FRHIError> Allocation = Allocator.Allocate( *Request.Requirements.ToAllocationDesc() );
        if ( not Allocation )
        {
            return TUnexpected<FRHIError>( Allocation.Error() );
        }
        Results.push_back( FRHITransientAllocation{ .Allocation=*Allocation, .ResourceId={}, .Lifetime=Request.Lifetime } );
        PlacedRequirements.push_back( Request.Requirements );
        Released.push_back( false );
    }
    return Results;
}

void Lumen::FRHITransientResourceAllocator::EndBatch () noexcept
{
    Allocator.Reset();
    bInBatch = false;
}
