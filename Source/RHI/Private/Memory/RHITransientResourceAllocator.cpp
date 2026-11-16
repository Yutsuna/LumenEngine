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
        return TUnexpected<FRHIError>(
            FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Memory, "Transient batch is not active" ) );
    }
    if ( const auto Valid = InBatch.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    TVector<FRHITransientAllocation> Results;
    TVector<Bool> Released;
    for ( const FRHITransientDesc &Request : InBatch.Requests )
    {
        auto Allocation = Allocator.Allocate( *Request.Requirements.ToAllocationDesc() );
        if ( not Allocation )
        {
            return TUnexpected<FRHIError>( Allocation.Error() );
        }
        FRHITransientAllocation Result{ *Allocation, {}, Request.Lifetime };
        for ( USize Index = 0U; Index < Results.size(); ++Index )
        {
            if ( not Released[Index] and not Results[Index].Lifetime.Overlaps( Request.Lifetime ) )
            {
                const auto Freed = Allocator.Free( Results[Index].Allocation );
                if ( not Freed )
                {
                    return TUnexpected<FRHIError>( Freed.Error() );
                }
                Released[Index] = true;
                break;
            }
        }
        Results.push_back( Result );
        Released.push_back( false );
    }
    return Results;
}
void Lumen::FRHITransientResourceAllocator::EndBatch () noexcept
{
    Allocator.Reset();
    bInBatch = false;
}
