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

Lumen::Tests::FNullQueue::FNullQueue ( const ERHIQueueType InType,
                                       const UInt32 InIndex,
                                       TSharedPtr<FNullFaultPlan> InFaultPlan,
                                       const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullQueue>() ), Type( InType ), FaultPlan( std::move( InFaultPlan ) )
{
    Identity.SetDebugName( std::format( "Null Queue %u", InIndex ) );
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullQueue )
Lumen::ERHIQueueType Lumen::Tests::FNullQueue::GetType () const noexcept
{
    return Type;
}

Lumen::FRHITimelineValue Lumen::Tests::FNullQueue::GetCompletedValue () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Completed;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullQueue::Submit ( const FRHISubmitDesc &InDesc ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( bLost )
    {
        return TUnexpected<FRHIError>( MakeRHIUnsupported( DeviceLost, ERHIErrorCondition::Queue, "Null queue is lost" ) );
    }
    if ( FaultPlan )
    {
        if ( auto Error = FaultPlan->ShouldFail( "Submit" ) )
        {
            return TUnexpected<FRHIError>( *Error );
        }
    }
    auto Valid = InDesc.Validate();
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
    for ( const auto &Wait : InDesc.Waits )
    {
        if ( Wait.Semaphore->GetDesc().Type == ERHISemaphoreType::Binary )
        {
            const auto Result = Wait.Semaphore->Wait( 1ULL, 0.0 );
            if ( not Result )
            {
                return Result;
            }
        }
    }
    for ( const auto &List : InDesc.CommandLists )
    {
        const auto Result = List->MarkPending( InDesc.Completion );
        if ( not Result )
        {
            return Result;
        }
    }
    for ( const auto &Signal : InDesc.Signals )
    {
        const auto Result = Signal.Semaphore->Signal( Signal.Value );
        if ( not Result )
        {
            return Result;
        }
    }
    Completed = InDesc.Completion;
    ++SubmitCount;
    for ( const auto &List : InDesc.CommandLists )
    {
        const auto Result = List->MarkCompleted( Completed );
        if ( not Result )
        {
            return Result;
        }
    }
    Condition.NotifyAll();
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullQueue::WaitIdle () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( bLost )
    {
        return TUnexpected<FRHIError>( MakeRHIUnsupported( DeviceLost, ERHIErrorCondition::Queue, "Null queue is lost" ) );
    }
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullQueue::Wait ( const FRHITimelineValue InValue,
                                                                          const Float64 InTimeoutSeconds ) noexcept
{
    if ( InTimeoutSeconds < 0.0 )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Negative queue timeout" ) );
    }
    HAL::TUniqueLock<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const auto Reached = [this, InValue]
    {
        return bLost or IsRHICompletionReached( Completed, InValue );
    };
    if ( not Condition.WaitFor( Lock, InTimeoutSeconds, Reached ) )
    {
        return TUnexpected<FRHIError>( MakeRHIUnsupported( Timeout, ERHIErrorCondition::Queue, "Queue wait timed out" ) );
    }
    if ( bLost )
    {
        return TUnexpected<FRHIError>( MakeRHIUnsupported( DeviceLost, ERHIErrorCondition::Queue, "Null queue is lost" ) );
    }
    return {};
}

void Lumen::Tests::FNullQueue::MarkLost () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    bLost = true;
    Condition.NotifyAll();
}

Lumen::UInt64 Lumen::Tests::FNullQueue::GetSubmitCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return SubmitCount;
}
