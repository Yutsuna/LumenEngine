/**
 * @file RHITransientAllocation.hpp
 * @brief Defines the result of transient placement.
 */
#pragma once
#include <Memory/RHIAllocation.hpp>
#include <Memory/RHIResourceLifetime.hpp>
#include <Templates/Optional.hpp>
namespace Lumen

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
} // namespace Lumen
