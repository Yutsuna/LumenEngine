/**
 * @file IRHIDefragmenter.hpp
 * @brief Declares defragmentation planning and execution.
 */
#pragma once
#include <Memory/RHIDefragPlan.hpp>
#include <Memory/Span.hpp>
#include <Types/IRHIObject.hpp>
namespace Lumen
{

/**
 * @class IRHIDefragmenter
 * @brief Produces and executes timeline-safe memory moves.
 */
class IRHIDefragmenter : public IRHIObject
{
public:

    /** @brief Builds a plan for the supplied live allocations. */
    [[nodiscard]] virtual TExpected<FRHIDefragPlan, FRHIError>
    BuildPlan ( TSpan<const FRHIAllocation> InAllocations ) noexcept = 0;
    /** @brief Schedules a previously validated plan. */
    [[nodiscard]] virtual TExpected<void, FRHIError> Execute ( const FRHIDefragPlan &InPlan ) noexcept = 0;
};
} // namespace Lumen
