/**
 * @file IRHITransientResourceAllocator.hpp
 * @brief Declares batched transient resource allocation.
 */
#pragma once
#include <Memory/RHITransientAllocation.hpp>
#include <Memory/RHITransientBatch.hpp>
#include <Types/IRHIObject.hpp>
namespace Lumen
{

/**
 * @class IRHITransientResourceAllocator
 * @brief Places non-overlapping lifetimes in reusable memory.
 */
class IRHITransientResourceAllocator : public IRHIObject
{
public:

    /** @brief Begins a batch scope. */
    virtual void BeginBatch () noexcept = 0;
    /**
     * @brief Places a batch of transient requests in reusable memory.
     * @param InBatch Requests with compatible requirements and lifetimes.
     * @return The placed allocations, or InvalidArgument / OutOfMemory on failure.
     */
    [[nodiscard]] virtual TExpected<TVector<FRHITransientAllocation>, FRHIError>
    Allocate ( const FRHITransientBatch &InBatch ) noexcept = 0;
    /** @brief Ends the current batch scope. */
    virtual void EndBatch () noexcept = 0;
};
} // namespace Lumen
