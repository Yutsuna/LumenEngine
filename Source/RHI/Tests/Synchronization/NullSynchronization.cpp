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

Lumen::Tests::FNullSemaphore::FNullSemaphore ( const FRHISemaphoreDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullSemaphore>() ), Desc( InDesc ), Value( InDesc.InitialValue )
{
    Identity.SetDebugName( InDesc.DebugName );
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullSemaphore )
const Lumen::FRHISemaphoreDesc &Lumen::Tests::FNullSemaphore::GetDesc () const noexcept
{
    return Desc;
}

Lumen::UInt64 Lumen::Tests::FNullSemaphore::GetValue () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Value;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullSemaphore::Signal ( const UInt64 InValue ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
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

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullSemaphore::Wait ( const UInt64 InValue,
                                                                              const Float64 InTimeoutSeconds ) noexcept
{
    if ( InTimeoutSeconds < 0.0 or ( Desc.Type == ERHISemaphoreType::Binary and InValue != 1ULL ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidSynchronization( "Invalid semaphore wait" ) );
    }
    HAL::TUniqueLock<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const auto Reached = [this, InValue]
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
