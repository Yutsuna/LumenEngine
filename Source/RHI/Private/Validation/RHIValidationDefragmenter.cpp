/**
 * @file RHIValidationDefragmenter.cpp
 * @brief Implements defragmentation plan validation.
 */

#include "RHIValidationDefragmenter.hpp"

Lumen::RHI::FRHIValidationDefragmenter::FRHIValidationDefragmenter ( TSharedRef<IRHIDefragmenter> InUnderlying,
                                                                     TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::Defragmenter );
}

Lumen::RHI::FRHIValidationDefragmenter::~FRHIValidationDefragmenter () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationDefragmenter )

Lumen::TExpected<Lumen::RHI::FRHIDefragPlan, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDefragmenter::BuildPlan ( const TSpan<const FRHIAllocation> InAllocations ) noexcept
{
    return Underlying->BuildPlan( InAllocations );
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDefragmenter::Execute ( const FRHIDefragPlan &InPlan ) noexcept
{
    if ( const TExpected<void, FRHIError> PlanValid = InPlan.Validate(); not PlanValid )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidDefragPlan, ERHIErrorCondition::Memory,
                                             "Defragmentation plan is invalid", this );
    }
    return Underlying->Execute( InPlan );
}
