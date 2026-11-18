/**
 * @file RHISemaphoreDesc.cpp
 * @brief Implements semaphore descriptor validation.
 */

#include <RHI/Synchronization/RHISemaphoreDesc.hpp>
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHISemaphoreDesc::Validate () const noexcept
{
    if ( Type > ERHISemaphoreType::Timeline or ( Type == ERHISemaphoreType::Binary and InitialValue > 1ULL ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidSynchronization( "Invalid semaphore type or initial value" ) );
    }
    return {};
}
