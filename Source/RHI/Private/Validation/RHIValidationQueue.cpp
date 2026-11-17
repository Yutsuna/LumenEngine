/**
 * @file RHIValidationQueue.cpp
 * @brief Implements validated queue submission and host waits.
 */
#include "RHIValidationQueue.hpp"
#include <Validation/RHIValidation.hpp>

Lumen::FRHIValidationQueue::FRHIValidationQueue ( TSharedRef<IRHIQueue> InUnderlying,
                                                  TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::Queue );
}

Lumen::FRHIValidationQueue::~FRHIValidationQueue () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationQueue )

Lumen::ERHIQueueType Lumen::FRHIValidationQueue::GetType () const noexcept
{
    return Underlying->GetType();
}

Lumen::FRHITimelineValue Lumen::FRHIValidationQueue::GetCompletedValue () const noexcept
{
    return Underlying->GetCompletedValue();
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationQueue::Submit ( const FRHISubmitDesc &InDesc ) noexcept
{
    auto Valid = InDesc.Validate();
    if ( not Valid )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidQueueSubmission, ERHIErrorCondition::Queue,
                                             Valid.Error().Message, this );
    }
    if ( InDesc.Completion <= GetCompletedValue() )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidQueueSubmission, ERHIErrorCondition::Queue,
                                             "Queue completion values must increase", this );
    }
    FRHISubmitDesc Unwrapped;
    Unwrapped.Completion = InDesc.Completion;
    Unwrapped.CommandLists.reserve( InDesc.CommandLists.size() );
    for ( const auto &List : InDesc.CommandLists )
    {
        auto Owned = State->ValidateObject( *List, ERHIValidationObjectKind::CommandList );
        if ( not Owned )
        {
            return Owned;
        }
        if ( List->GetQueueType() != GetType() )
        {
            return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidQueueSubmission, ERHIErrorCondition::Queue,
                                                 "Command list queue type mismatch", this );
        }
        Unwrapped.CommandLists.push_back( UnwrapRHIShared( List ) );
    }
    Unwrapped.Waits = InDesc.Waits;
    for ( auto &Wait : Unwrapped.Waits )
    {
        Wait.Semaphore = UnwrapRHIShared( Wait.Semaphore );
        if ( Wait.Semaphore->GetValue() < Wait.Value )
        {
            return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidQueueSubmission,
                                                 ERHIErrorCondition::Synchronization, "Submission wait is not signaled", this );
        }
    }
    Unwrapped.Signals = InDesc.Signals;
    for ( auto &Signal : Unwrapped.Signals )
    {
        Signal.Semaphore = UnwrapRHIShared( Signal.Semaphore );
    }
    return Underlying->Submit( Unwrapped );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationQueue::WaitIdle () noexcept
{
    return Underlying->WaitIdle();
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationQueue::Wait ( const FRHITimelineValue InValue,
                                                                            const Float64 InTimeoutSeconds ) noexcept
{
    return Underlying->Wait( InValue, InTimeoutSeconds );
}
