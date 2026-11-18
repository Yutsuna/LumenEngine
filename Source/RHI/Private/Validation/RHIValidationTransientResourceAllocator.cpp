/**
 * @file RHIValidationTransientResourceAllocator.cpp
 * @brief Implements transient batch and lifetime validation.
 */

#include "RHIValidationTransientResourceAllocator.hpp"
#include <RHI/Utilities/RHIAlignment.hpp>

Lumen::RHI::FRHIValidationTransientResourceAllocator::FRHIValidationTransientResourceAllocator (
    TSharedRef<IRHITransientResourceAllocator> InUnderlying, TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::TransientAllocator );
}

Lumen::RHI::FRHIValidationTransientResourceAllocator::~FRHIValidationTransientResourceAllocator () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationTransientResourceAllocator )

void Lumen::RHI::FRHIValidationTransientResourceAllocator::BeginBatch () noexcept
{
    if ( bInBatch and State->GetReporter()->GetDesc().IsEnabled( ERHIValidationFlags::TransientResources ) )
    {
        State->GetReporter()->Report( ERHIValidationMessageCode::InvalidTransientBatch, "Transient batch is already active",
                                      this );
        return;
    }
    Underlying->BeginBatch();
    ActiveAllocations.clear();
    bInBatch = true;
}

Lumen::TExpected<Lumen::TVector<Lumen::RHI::FRHITransientAllocation>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationTransientResourceAllocator::Allocate ( const FRHITransientBatch &InBatch ) noexcept
{
    const Bool bValidate = State->GetReporter()->GetDesc().IsEnabled( ERHIValidationFlags::TransientResources );
    if ( bValidate and not bInBatch )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::InvalidTransientBatch, ERHIErrorCondition::Memory,
                                                     "Transient batch is not active", this )
                                           .Error() );
    }
    if ( bValidate )
    {
        const TExpected<void, FRHIError> DescValid = InBatch.Validate();
        if ( not DescValid )
        {
            return TUnexpected<FRHIError>( State->GetReporter()
                                               ->Reject( ERHIValidationMessageCode::InvalidTransientBatch,
                                                         ERHIErrorCondition::Memory, DescValid.Error().Message, this )
                                               .Error() );
        }
    }
    return Underlying->Allocate( InBatch ).AndThen(
        [this, &InBatch, bValidate] (
            TVector<FRHITransientAllocation> InAllocations ) noexcept -> TExpected<TVector<FRHITransientAllocation>, FRHIError>
    {
        if ( bValidate and InAllocations.size() != InBatch.Requests.size() )
        {
            return TUnexpected<FRHIError>( State->GetReporter()
                                               ->Reject( ERHIValidationMessageCode::InvalidTransientBatch,
                                                         ERHIErrorCondition::Memory,
                                                         "Backend returned the wrong transient allocation count", this )
                                               .Error() );
        }
        if ( bValidate )
        {
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
        }
        for ( const FRHITransientAllocation &Allocation : InAllocations )
        {
            ActiveAllocations.push_back( Allocation );
        }
        return InAllocations;
    } );
}

void Lumen::RHI::FRHIValidationTransientResourceAllocator::EndBatch () noexcept
{
    if ( not bInBatch and State->GetReporter()->GetDesc().IsEnabled( ERHIValidationFlags::TransientResources ) )
    {
        State->GetReporter()->Report( ERHIValidationMessageCode::InvalidTransientBatch, "No transient batch is active", this );
        return;
    }
    Underlying->EndBatch();
    ActiveAllocations.clear();
    bInBatch = false;
}
