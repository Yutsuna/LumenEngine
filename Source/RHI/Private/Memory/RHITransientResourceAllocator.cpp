/**
 * @file RHITransientResourceAllocator.cpp
 * @brief Implements transient batches and disjoint-lifetime aliasing.
 */
#include "RHITransientResourceAllocator.hpp"
#include <Templates/Hash.hpp>
Lumen::FRHITransientResourceAllocator::FRHITransientResourceAllocator ( const FRHIObjectId InId,
                                                                        const UInt64 InCapacity ) noexcept
    : Identity( InId, FHash::TypeHash<FRHITransientResourceAllocator>() ), Allocator( InCapacity )
{
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
    if ( const auto Valid = InBatch.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    TVector<FRHITransientAllocation> Results;
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
            if ( not bOverlapsRemainingRequest )
            {
                const auto Freed = Allocator.Free( Results[Index].Allocation );
                if ( not Freed )
                {
                    return TUnexpected<FRHIError>( Freed.Error() );
                }
                Released[Index] = true;
            }
        }
        auto Allocation = Allocator.Allocate( *Request.Requirements.ToAllocationDesc() );
        if ( not Allocation )
        {
            return TUnexpected<FRHIError>( Allocation.Error() );
        }
        Results.push_back( FRHITransientAllocation{ *Allocation, {}, Request.Lifetime } );
        Released.push_back( false );
    }
    return Results;
}
void Lumen::FRHITransientResourceAllocator::EndBatch () noexcept
{
    Allocator.Reset();
    bInBatch = false;
}
