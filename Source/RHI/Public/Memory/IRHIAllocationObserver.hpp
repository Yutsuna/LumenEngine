/**
 * @file IRHIAllocationObserver.hpp
 * @brief Declares allocation lifecycle notifications.
 */
#pragma once
#include <Memory/RHIAllocation.hpp>
namespace Lumen
{

/**
 * @class IRHIAllocationObserver
 * @brief Observer used by accounting, diagnostics, and defragmentation.
 */
class IRHIAllocationObserver
{
public:

    virtual ~IRHIAllocationObserver () noexcept = default;
    /** @brief Called after a successful allocation. */
    virtual void OnAllocationCreated ( const FRHIAllocation &InAllocation ) noexcept = 0;
    /** @brief Called before an allocation is released. */
    virtual void OnAllocationReleased ( const FRHIAllocation &InAllocation ) noexcept = 0;
    /** @brief Called after a defragmentation move. */
    virtual void OnAllocationMoved ( const FRHIAllocation &InBefore, const FRHIAllocation &InAfter ) noexcept = 0;
};
} // namespace Lumen
