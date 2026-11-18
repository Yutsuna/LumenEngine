/**
 * @file IRHIAllocator.hpp
 * @brief Declares the address allocator SPI.
 */
#pragma once

#include <Memory/RHIAllocation.hpp>
#include <Memory/RHIAllocationDesc.hpp>
#include <Memory/RHIAllocatorStats.hpp>
#include <Templates/Expected.hpp>

namespace Lumen
{

/**
 * @class IRHIAllocator
 * @brief Backend-independent address allocation contract.
 */
class IRHIAllocator
{
public:

    virtual ~IRHIAllocator () noexcept = default;
    /** @brief Allocates a non-overlapping range satisfying InDesc. */
    [[nodiscard]] virtual TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept = 0;
    /** @brief Releases a range returned by this allocator. */
    [[nodiscard]] virtual TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept = 0;
    /** @brief Releases all ranges where the allocator supports global reset. */
    virtual void Reset () noexcept = 0;
    /** @return Current statistics snapshot. */
    [[nodiscard]] virtual FRHIAllocatorStats GetStats () const noexcept = 0;
};
} // namespace Lumen
