/**
 * @file NullQueue.cpp
 * @brief Implements serialized deterministic Null queues.
 */

#include <HAL/LockGuard.hpp>
#include <Queue/NullQueue.hpp>
#include <Templates/Hash.hpp>

#include <format>

/**
 * Public
 */

Lumen::RHI::Tests::FNullQueue::FNullQueue ( const ERHIQueueType InType,
                                            const UInt32 InIndex,
                                            TSharedPtr<FNullFaultPlan> InFaultPlan,
                                            const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullQueue>() ), Type( InType ), FaultPlan( std::move( InFaultPlan ) )
{
    Identity.SetDebugName( std::format( "Null Queue %u", InIndex ) );
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullQueue )
Lumen::RHI::ERHIQueueType Lumen::RHI::Tests::FNullQueue::GetType () const noexcept
{
    return Type;
}

Lumen::RHI::FRHITimelineValue Lumen::RHI::Tests::FNullQueue::GetCompletedValue () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Completed;
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullQueue::Submit ( const FRHISubmitDesc &InDesc ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( bLost )
    {
        return TUnexpected<FRHIError>( MakeRHIDeviceLost( ERHIErrorCondition::Queue, "Null queue is lost" ) );
    }
    if ( FaultPlan )
    {
        if ( const TOptional<FRHIError> Error = FaultPlan->ShouldFail( "Submit" ) )
        {
            return TUnexpected<FRHIError>( *Error );
        }
    }
    TExpected<void, FRHIError> Valid = InDesc.Validate();
    if ( not Valid )
    {
        return Valid;
    }
    if ( InDesc.Completion <= Completed )
    {
        return TUnexpected<FRHIError>(
            FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Queue, "Queue completion values must increase" ) );
    }
    for ( const auto &List : InDesc.CommandLists )
    {
        if ( List->GetQueueType() != Type )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Command list queue type mismatch" ) );
        }
    }
    for ( const auto &Wait : InDesc.Waits )
    {
        if ( Wait.Semaphore->GetValue() < Wait.Value )
        {
            return TUnexpected<FRHIError>(
                FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Queue, "Null submission wait is not signaled" ) );
        }
    }
    for ( const auto &Signal : InDesc.Signals )
    {
        const auto &SemaphoreDesc = Signal.Semaphore->GetDesc();
        if ( ( SemaphoreDesc.Type == ERHISemaphoreType::Binary and Signal.Value != 1ULL ) or
             ( SemaphoreDesc.Type == ERHISemaphoreType::Timeline and Signal.Value < Signal.Semaphore->GetValue() ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidState( ERHIErrorCondition::Synchronization, "Null submission signal would be rejected" ) );
        }
    }
    for ( const auto &Wait : InDesc.Waits )
    {
        if ( Wait.Semaphore->GetDesc().Type == ERHISemaphoreType::Binary )
        {
            const TExpected<void, FRHIError> Result = Wait.Semaphore->Wait( 1ULL, 0.0 );
            if ( not Result )
            {
                return Result;
            }
        }
    }
    for ( const auto &Signal : InDesc.Signals )
    {
        const TExpected<void, FRHIError> Result = Signal.Semaphore->Signal( Signal.Value );
        if ( not Result )
        {
            return Result;
        }
    }
    for ( const auto &List : InDesc.CommandLists )
    {
        const TExpected<void, FRHIError> Result = List->MarkPending( InDesc.Completion );
        if ( not Result )
        {
            return Result;
        }
    }
    Completed = InDesc.Completion;
    ++SubmitCount;
    for ( const auto &List : InDesc.CommandLists )
    {
        const TExpected<void, FRHIError> Result = List->MarkCompleted( Completed );
        if ( not Result )
        {
            return Result;
        }
    }
    Condition.NotifyAll();
    return {};
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullQueue::WaitIdle () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( bLost )
    {
        return TUnexpected<FRHIError>( MakeRHIDeviceLost( ERHIErrorCondition::Queue, "Null queue is lost" ) );
    }
    return {};
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullQueue::Wait ( const FRHITimelineValue InValue,
                                                                                    const Float64 InTimeoutSeconds ) noexcept
{
    if ( InTimeoutSeconds < 0.0 )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Negative queue timeout" ) );
    }
    HAL::TUniqueLock<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TFunction<Bool()> Reached = [this, InValue]
    {
        return bLost or IsRHICompletionReached( Completed, InValue );
    };
    if ( not Condition.WaitFor( Lock, InTimeoutSeconds, Reached ) )
    {
        return TUnexpected<FRHIError>( MakeRHITimeout( ERHIErrorCondition::Queue, "Queue wait timed out" ) );
    }
    if ( bLost )
    {
        return TUnexpected<FRHIError>( MakeRHIDeviceLost( ERHIErrorCondition::Queue, "Null queue is lost" ) );
    }
    return {};
}

void Lumen::RHI::Tests::FNullQueue::MarkLost () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    bLost = true;
    Condition.NotifyAll();
}

Lumen::UInt64 Lumen::RHI::Tests::FNullQueue::GetSubmitCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return SubmitCount;
}
