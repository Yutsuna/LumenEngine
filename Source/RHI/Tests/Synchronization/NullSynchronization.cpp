/**
 * @file NullSynchronization.cpp
 * @brief Implements deterministic Null semaphores.
 */

#include <HAL/LockGuard.hpp>
#include <Synchronization/NullSynchronization.hpp>
#include <Templates/Hash.hpp>

/**
 * Public
 */

Lumen::RHI::Tests::FNullSemaphore::FNullSemaphore ( const FRHISemaphoreDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullSemaphore>() ), Desc( InDesc ), Value( InDesc.InitialValue )
{
    Identity.SetDebugName( InDesc.DebugName );
}

LUMEN_RHI_OBJECT_FORWARDERS( RHI::Tests, FNullSemaphore )
const Lumen::RHI::FRHISemaphoreDesc &Lumen::RHI::Tests::FNullSemaphore::GetDesc () const noexcept
{
    return Desc;
}

Lumen::UInt64 Lumen::RHI::Tests::FNullSemaphore::GetValue () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Value;
}

void Lumen::RHI::Tests::FNullSemaphore::SetFailNextSignal ( const Bool bInFailNextSignal ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    SignalFaultCount = bInFailNextSignal ? 1U : 0U;
}

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullSemaphore::Signal ( const UInt64 InValue ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( SignalFaultCount > 0U )
    {
        --SignalFaultCount;
        return TUnexpected<FRHIError>(
            FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Synchronization, "Semaphore signal fault injected" ) );
    }
    if ( ( Desc.Type == ERHISemaphoreType::Binary and InValue != 1ULL ) or
         ( Desc.Type == ERHISemaphoreType::Timeline and InValue < Value ) )
    {
        return TUnexpected<FRHIError>( FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Synchronization,
                                                  "Semaphore signal would regress or is not binary" ) );
    }
    Value = InValue;
    Condition.NotifyAll();
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullSemaphore::Wait ( const UInt64 InValue, const Float64 InTimeoutSeconds ) noexcept
{
    if ( InTimeoutSeconds < 0.0 or ( Desc.Type == ERHISemaphoreType::Binary and InValue != 1ULL ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidSynchronization( "Invalid semaphore wait" ) );
    }
    HAL::TUniqueLock<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TFunction<Bool()> Reached = [this, InValue]
    {
        return Value >= InValue;
    };
    if ( not Condition.WaitFor( Lock, InTimeoutSeconds, Reached ) )
    {
        return TUnexpected<FRHIError>( MakeRHITimeout( ERHIErrorCondition::Synchronization, "Semaphore wait timed out" ) );
    }
    if ( Desc.Type == ERHISemaphoreType::Binary )
    {
        Value = 0ULL;
    }
    return {};
}
