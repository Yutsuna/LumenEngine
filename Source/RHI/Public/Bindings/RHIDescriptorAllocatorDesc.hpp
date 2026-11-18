/**
 * @file RHIDescriptorAllocatorDesc.hpp
 * @brief Defines descriptor allocator capacity policy.
 */

#pragma once
#include <RHIExport.hpp>
#include <Types/RHIError.hpp>
namespace Lumen

{
/**
 * @struct FRHIDescriptorAllocatorDesc
 * @brief Initial and maximum descriptor counts.
 */
struct RHI_EXPORT FRHIDescriptorAllocatorDesc
{
    UInt32 InitialCapacity = 256U;
    UInt32 MaximumCapacity = 4096U;
    /** @return Success for nonzero ordered capacities. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
