/**
 * @file NullFaultPlan.cpp
 * @brief Implements deterministic Null fault injection.
 */

#include <Device/NullFaultPlan.hpp>
#include <HAL/LockGuard.hpp>

void Lumen::RHI::Tests::FNullFaultPlan::AddFailure ( const FStringView InOperation,
                                                     const UInt32 InOccurrence,
                                                     const FRHIError &InError ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Rules.push_back(
        FRule{ .Operation = FString( InOperation ), .Occurrence = InOccurrence == 0U ? 1U : InOccurrence, .Error = InError } );
}

Lumen::TOptional<Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullFaultPlan::ShouldFail ( const FStringView InOperation ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( FRule &Rule : Rules )
    {
        if ( Rule.Operation == InOperation )
        {
            ++Rule.Calls;
            if ( Rule.Calls == Rule.Occurrence )
            {
                return TOptional<FRHIError>( Rule.Error );
            }
        }
    }
    return {};
}
