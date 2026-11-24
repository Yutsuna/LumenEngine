/**
 * @file RHISubmitDesc.cpp
 * @brief Implements submission descriptor validation.
 */

#include <RHI/Queue/RHISubmitDesc.hpp>
Lumen::RHI::FRHIExpected Lumen::RHI::FRHISubmitDesc::Validate () const noexcept
{
    if ( not Completion.IsValid() or ( CommandLists.empty() and Signals.empty() ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Submission has no work or completion value" ) );
    }
    for ( const auto &List : CommandLists )
    {
        if ( List->GetState() != ERHICommandListState::Executable )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidState( ERHIErrorCondition::Queue, "Only executable command lists can be submitted" ) );
        }
    }
    for ( USize First = 0U; First < Signals.size(); ++First )
    {
        const auto &Signal = Signals[First];
        if ( not Signal.Semaphore or Signal.Value == 0ULL or
             ( Signal.Semaphore->GetDesc().Type == ERHISemaphoreType::Binary and Signal.Value != 1ULL ) or
             ( Signal.Semaphore->GetDesc().Type == ERHISemaphoreType::Timeline and
               Signal.Value <= Signal.Semaphore->GetValue() ) )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Invalid semaphore signal" ) );
        }
        for ( USize Second = First + 1U; Second < Signals.size(); ++Second )
        {
            if ( Signals[Second].Semaphore.Get() == Signal.Semaphore.Get() )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Duplicate semaphore signal" ) );
            }
        }
    }
    for ( const auto &Wait : Waits )
    {
        if ( not Wait.Semaphore or ( Wait.Semaphore->GetDesc().Type == ERHISemaphoreType::Binary and Wait.Value != 1ULL ) )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Invalid semaphore wait" ) );
        }
    }
    return {};
}
