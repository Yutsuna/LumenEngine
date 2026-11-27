/**
 * @file IRHIDefragmenter.hpp
 * @brief Declares defragmentation planning and execution.
 */
#pragma once

#include <Memory/Span.hpp>
#include <RHI/Memory/RHIDefragPlan.hpp>
#include <RHI/Types/IRHIObject.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class IRHIDefragmenter
     * @brief Produces and executes timeline-safe memory moves.
     */
    class IRHIDefragmenter : public IRHIObject
    {
    public:

        /**
         * @brief Builds a validated plan of non-overlapping moves.
         * @param InAllocations Live allocations to be relocated.
         * @return The plan, or InvalidArgument for overlapping or pinned inputs.
         */
        [[nodiscard]] virtual TExpected<FRHIDefragPlan, FRHIError>
        BuildPlan ( TSpan<const FRHIAllocation> InAllocations ) noexcept = 0;
        /** @brief Schedules a previously validated plan. */
        [[nodiscard]] virtual FRHIExpected Execute ( const FRHIDefragPlan &InPlan ) noexcept = 0;
    };
} // namespace RHI

} // namespace Lumen
