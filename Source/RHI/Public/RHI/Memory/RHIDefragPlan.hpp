/**
 * @file RHIDefragPlan.hpp
 * @brief Defines a validated defragmentation plan.
 */
#pragma once

#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHITimeline.hpp>
#include <RHIExport.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHIDefragMove
     * @brief One source-to-destination allocation move.
     * @details Completion names the timeline value that must be reached before the
     *          move is safe to execute.
     */
    struct FRHIDefragMove
    {
        FRHIAllocation Source;
        FRHIAllocation Destination;
        FRHITimelineValue Completion = {};
    };
    /**
     * @struct FRHIDefragPlan
     * @brief Ordered moves and estimated reclaimed bytes.
     * @details The caller advances ObservedCompletion as GPU work retires; a move is
     *          executed only once that value reaches its Completion value.
     */
    struct RHI_EXPORT FRHIDefragPlan
    {
        TVector<FRHIDefragMove> Moves;
        UInt64 ReclaimedBytes                = 0ULL;
        FRHITimelineValue ObservedCompletion = {};
        /** @return Success when every source/destination is valid and ranges do not overlap incorrectly. */
        [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    };
} // namespace RHI

} // namespace Lumen
