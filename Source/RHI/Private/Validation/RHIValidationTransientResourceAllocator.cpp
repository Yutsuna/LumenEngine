/**
 * @file RHIValidationTransientResourceAllocator.cpp
 * @brief Implements transient batch and lifetime validation.
 */
#include "RHIValidationTransientResourceAllocator.hpp"
#include <Utilities/RHIAlignment.hpp>

Lumen::FRHIValidationTransientResourceAllocator::FRHIValidationTransientResourceAllocator (
    TSharedRef<IRHITransientResourceAllocator> InUnderlying, TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::TransientAllocator );
}

Lumen::FRHIValidationTransientResourceAllocator::~FRHIValidationTransientResourceAllocator () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationTransientResourceAllocator )

void Lumen::FRHIValidationTransientResourceAllocator::BeginBatch () noexcept
{
    if ( bInBatch )
    {
        State->GetReporter()->Report( ERHIValidationMessageCode::InvalidTransientBatch, "Transient batch is already active",
                                      this );
        return;
    }
    Underlying->BeginBatch();
    ActiveAllocations.clear();
    bInBatch = true;
}

Lumen::TExpected<Lumen::TVector<Lumen::FRHITransientAllocation>, Lumen::FRHIError>
Lumen::FRHIValidationTransientResourceAllocator::Allocate ( const FRHITransientBatch &InBatch ) noexcept
{
    if ( not bInBatch )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::InvalidTransientBatch, ERHIErrorCondition::Memory,
                                                     "Transient batch is not active", this )
                                           .Error() );
    }
    return InBatch.Validate()
        .OrElse(
            [this] ( const FRHIError &InError ) noexcept -> TExpected<void, FRHIError>
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidTransientBatch, ERHIErrorCondition::Memory,
                                             InError.Message, this );
    } )
        .AndThen(
            [this, &InBatch] () noexcept -> TExpected<TVector<FRHITransientAllocation>, FRHIError>
    {
        return Underlying->Allocate( InBatch );
    } )
        .AndThen( [this, &InBatch] ( TVector<FRHITransientAllocation> InAllocations ) noexcept
                      -> TExpected<TVector<FRHITransientAllocation>, FRHIError>
    {
        if ( InAllocations.size() != InBatch.Requests.size() )
        {
            return TUnexpected<FRHIError>( State->GetReporter()
                                               ->Reject( ERHIValidationMessageCode::InvalidTransientBatch,
                                                         ERHIErrorCondition::Memory,
                                                         "Backend returned the wrong transient allocation count", this )
                                               .Error() );
        }
        for ( USize Index = 0U; Index < InAllocations.size(); ++Index )
        {
            const FRHITransientAllocation &Allocation = InAllocations[Index];
            if ( not Allocation.Allocation.IsValid() or Allocation.Lifetime.Begin != InBatch.Requests[Index].Lifetime.Begin or
                 Allocation.Lifetime.End != InBatch.Requests[Index].Lifetime.End )
            {
                return TUnexpected<FRHIError>( State->GetReporter()
                                                   ->Reject( ERHIValidationMessageCode::InvalidTransientBatch,
                                                             ERHIErrorCondition::Memory,
                                                             "Backend returned an invalid transient lifetime", this )
                                                   .Error() );
            }
            for ( const FRHITransientAllocation &Active : ActiveAllocations )
            {
                if ( Active.Lifetime.Overlaps( Allocation.Lifetime ) and
                     RHIRangesOverlap( Active.Allocation.Offset, Active.Allocation.Size, Allocation.Allocation.Offset,
                                       Allocation.Allocation.Size ) )
                {
                    return TUnexpected<FRHIError>( State->GetReporter()
                                                       ->Reject( ERHIValidationMessageCode::InvalidTransientBatch,
                                                                 ERHIErrorCondition::Memory,
                                                                 "Overlapping transient lifetimes alias memory", this )
                                                       .Error() );
                }
            }
        }
        for ( const FRHITransientAllocation &Allocation : InAllocations )
        {
            ActiveAllocations.push_back( Allocation );
        }
        return InAllocations;
    } );
}

void Lumen::FRHIValidationTransientResourceAllocator::EndBatch () noexcept
{
    if ( not bInBatch )
    {
        State->GetReporter()->Report( ERHIValidationMessageCode::InvalidTransientBatch, "No transient batch is active", this );
        return;
    }
    Underlying->EndBatch();
    ActiveAllocations.clear();
    bInBatch = false;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationTransientResourceAllocator::ValidateResourceUse ( const FRHITransientAllocation &InAllocation,
                                                                       const UInt64 InFrame ) const noexcept
{
    if ( not bInBatch or InFrame < InAllocation.Lifetime.Begin or InFrame >= InAllocation.Lifetime.End )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidTransientBatch, ERHIErrorCondition::Memory,
                                             "Transient resource is outside its active lifetime", this );
    }
    for ( const FRHITransientAllocation &Active : ActiveAllocations )
    {
        if ( Active.Allocation.Id == InAllocation.Allocation.Id )
        {
            return {};
        }
    }
    return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidTransientBatch, ERHIErrorCondition::Memory,
                                         "Transient allocation is foreign or expired", this );
}
