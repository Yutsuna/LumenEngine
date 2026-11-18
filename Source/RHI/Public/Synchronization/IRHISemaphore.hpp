/**
 * @file IRHISemaphore.hpp
 * @brief Declares binary and monotonic timeline synchronization.
 */

#pragma once

#include <Synchronization/RHISemaphoreDesc.hpp>
#include <Types/IRHIObject.hpp>

namespace Lumen
{

/**
 * @class IRHISemaphore
 * @brief Binary or timeline synchronization primitive owned by the device.
 */
class IRHISemaphore : public IRHIObject
{
public:

    /**
     * @brief Returns the immutable semaphore descriptor.
     * @return A reference owned by the semaphore.
     */
    [[nodiscard]] virtual const FRHISemaphoreDesc &GetDesc () const noexcept = 0;
    /**
     * @brief Returns the current semaphore value.
     * @return Latest signaled value; timeline values never regress.
     */
    [[nodiscard]] virtual UInt64 GetValue () const noexcept = 0;
    /** @brief Timeline signals never regress; binary signals use one. */
    virtual TExpected<void, FRHIError> Signal ( UInt64 InValue ) noexcept = 0;
    /** @brief Waits for a timeline value, or consumes a signaled binary semaphore. */
    virtual TExpected<void, FRHIError> Wait ( UInt64 InValue, Float64 InTimeoutSeconds ) noexcept = 0;
};
} // namespace Lumen
