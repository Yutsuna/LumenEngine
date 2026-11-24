/**
 * @file RHIValidationQueue.cpp
 * @brief Implements validated queue submission and host waits.
 */

#include "RHIValidationQueue.hpp"
#include <RHI/Validation/RHIValidation.hpp>

Lumen::RHI::FRHIValidationQueue::FRHIValidationQueue ( TSharedRef<IRHIQueue> InUnderlying,
                                                       TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::Queue );
}

Lumen::RHI::FRHIValidationQueue::~FRHIValidationQueue () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationQueue )

Lumen::RHI::ERHIQueueType Lumen::RHI::FRHIValidationQueue::GetType () const noexcept
{
    return Underlying->GetType();
}

Lumen::RHI::FRHITimelineValue Lumen::RHI::FRHIValidationQueue::GetCompletedValue () const noexcept
{
    return Underlying->GetCompletedValue();
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationQueue::Submit ( const FRHISubmitDesc &InDesc ) noexcept
{
    const Bool bValidate = State->GetReporter()->GetDesc().IsEnabled( ERHIValidationFlags::Queues );
    if ( bValidate )
    {
        const FRHIExpected DescValid = InDesc.Validate();
        if ( not DescValid )
        {
            return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidQueueSubmission, ERHIErrorCondition::Queue,
                                                 DescValid.Error().Message, this );
        }
        if ( InDesc.Completion <= GetCompletedValue() )
        {
            return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidQueueSubmission, ERHIErrorCondition::Queue,
                                                 "Queue completion values must increase", this );
        }
    }
    FRHISubmitDesc Unwrapped;
    Unwrapped.Completion = InDesc.Completion;
    Unwrapped.CommandLists.reserve( InDesc.CommandLists.size() );
    for ( const TSharedRef<IRHICommandList> &List : InDesc.CommandLists )
    {
        if ( bValidate )
        {
            const FRHIExpected Owned = State->ValidateObject( *List, ERHIValidationObjectKind::CommandList );
            if ( not Owned )
            {
                return Owned;
            }
            if ( List->GetQueueType() != GetType() )
            {
                return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidQueueSubmission, ERHIErrorCondition::Queue,
                                                     "Command list queue type mismatch", this );
            }
        }
        Unwrapped.CommandLists.push_back( UnwrapRHIShared( List ) );
    }
    Unwrapped.Waits = InDesc.Waits;
    for ( FRHISemaphoreOperation &Wait : Unwrapped.Waits )
    {
        Wait.Semaphore = UnwrapRHIShared( Wait.Semaphore );
        if ( bValidate and Wait.Semaphore->GetValue() < Wait.Value )
        {
            return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidQueueSubmission, ERHIErrorCondition::Queue,
                                                 "Submission wait is not signaled", this );
        }
    }
    Unwrapped.Signals = InDesc.Signals;
    for ( FRHISemaphoreOperation &Signal : Unwrapped.Signals )
    {
        Signal.Semaphore = UnwrapRHIShared( Signal.Semaphore );
    }
    return Underlying->Submit( Unwrapped );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationQueue::WaitIdle () noexcept
{
    return Underlying->WaitIdle();
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationQueue::Wait ( const FRHITimelineValue InValue,
                                                                 const Float64 InTimeoutSeconds ) noexcept
{
    return Underlying->Wait( InValue, InTimeoutSeconds );
}
