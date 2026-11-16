/**
 * @file RHISemaphoreDesc.cpp
 * @brief Implements semaphore descriptor validation.
 */

#include <Synchronization/RHISemaphoreDesc.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHISemaphoreDesc::Validate () const noexcept
{
    if ( Type > ERHISemaphoreType::Timeline or ( Type == ERHISemaphoreType::Binary and InitialValue > 1ULL ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Synchronization, "Invalid semaphore type or initial value" ) );
    }
    return {};
}
