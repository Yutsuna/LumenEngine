/**
 * @file RHIMemoryAllocator.hpp
 * @brief Declares the budgeted allocator facade.
 */
#pragma once
#include <Memory/IRHIAllocationObserver.hpp>
#include <Memory/IRHIAllocator.hpp>
#include <Memory/RHIMemoryBudget.hpp>
#include <RHIExport.hpp>
namespace Lumen
{

/**
 * @class FRHIMemoryAllocator
 * @brief Combines an address allocator with budget accounting.
 * @details Observers registered on the facade are notified after a successful
 *          allocation or release; failed operations leave observers untouched.
 */
class RHI_EXPORT FRHIMemoryAllocator final
{
public:

    /** @brief Uses existing allocator and budget without taking ownership. */
    FRHIMemoryAllocator ( IRHIAllocator &InAllocator, FRHIMemoryBudget &InBudget ) noexcept;
    /** @brief Adds an observer notified of subsequent allocation lifecycle events. */
    void RegisterObserver ( IRHIAllocationObserver &InObserver ) noexcept;
    /** @brief Removes a previously registered observer. */
    void UnregisterObserver ( IRHIAllocationObserver &InObserver ) noexcept;
    /** @brief Reserves budget and allocates; failure releases the reservation. */
    [[nodiscard]] TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept;
    /** @brief Frees a range and releases its budget. */
    [[nodiscard]] TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept;

private:

    IRHIAllocator *Allocator;
    FRHIMemoryBudget *Budget;
};
} // namespace Lumen
