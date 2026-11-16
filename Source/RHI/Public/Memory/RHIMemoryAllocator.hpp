/**
 * @file RHIMemoryAllocator.hpp
 * @brief Declares the budgeted allocator facade.
 */
#pragma once
#include <Memory/IRHIAllocator.hpp>
#include <Memory/RHIMemoryBudget.hpp>
#include <RHIExport.hpp>
namespace Lumen
{

/**
 * @class FRHIMemoryAllocator
 * @brief Combines an address allocator with budget accounting.
 */
class RHI_EXPORT FRHIMemoryAllocator final
{
public:

    /** @brief Uses existing allocator and budget without taking ownership. */
    FRHIMemoryAllocator ( IRHIAllocator &InAllocator, FRHIMemoryBudget &InBudget ) noexcept;
    /** @brief Reserves budget and allocates; failure releases the reservation. */
    [[nodiscard]] TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept;
    /** @brief Frees a range and releases its budget. */
    [[nodiscard]] TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept;

private:

    IRHIAllocator *Allocator;
    FRHIMemoryBudget *Budget;
};
} // namespace Lumen
