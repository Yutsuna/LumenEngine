/**
 * @file RHIDescriptorAllocatorDesc.hpp
 * @brief Defines descriptor allocator capacity policy.
 */

#pragma once

#include <RHI/Types/RHIError.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
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
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
