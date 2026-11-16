/**
 * @file RHIDefragPlan.hpp
 * @brief Defines a validated defragmentation plan.
 */
#pragma once
#include <Memory/RHIAllocation.hpp>
#include <RHIExport.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHIError.hpp>
namespace Lumen

{
/**
 * @struct FRHIDefragMove
 * @brief One source-to-destination allocation move.
 */
struct FRHIDefragMove
{
    FRHIAllocation Source;
    FRHIAllocation Destination;
};
/**
 * @struct FRHIDefragPlan
 * @brief Ordered moves and estimated reclaimed bytes.
 */
struct RHI_EXPORT FRHIDefragPlan
{
    TVector<FRHIDefragMove> Moves;
    UInt64 ReclaimedBytes = 0ULL;
    /** @return Success when every source/destination is valid and ranges do not overlap incorrectly. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
