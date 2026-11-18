/**
 * @file RHITransientAllocation.hpp
 * @brief Defines the result of transient placement.
 */
#pragma once

#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Memory/RHIResourceLifetime.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHI
{
    /**
     * @struct FRHITransientAllocation
     * @brief Placed range, optional resource identity, and lifetime.
     */
    struct FRHITransientAllocation
    {
        FRHIAllocation Allocation;
        TOptional<FRHIObjectId> ResourceId;
        FRHIResourceLifetime Lifetime;
    };
} // namespace RHI

} // namespace Lumen
