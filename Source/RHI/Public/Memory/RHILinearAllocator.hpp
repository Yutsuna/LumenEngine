/**
 * @file RHILinearAllocator.hpp
 * @brief Declares a bounded bump allocator.
 */
#pragma once
#include <HAL/PlatformMutex.hpp>
#include <Memory/IRHIAllocator.hpp>
#include <RHIExport.hpp>
namespace Lumen
{

/**
 * @class FRHILinearAllocator
 * @brief Constant-time allocation with global reset.
 */
class RHI_EXPORT FRHILinearAllocator final : public IRHIAllocator
{
public:

    /** @brief Creates an allocator over InCapacity bytes. */
    explicit FRHILinearAllocator ( UInt64 InCapacity ) noexcept;
    TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;
    TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept override;
    void Reset () noexcept override;
    FRHIAllocatorStats GetStats () const noexcept override;

private:

    UInt64 Capacity;
    UInt64 Current = 0ULL;
    UInt64 NextId  = 1ULL;
    FRHIAllocatorStats Stats;
    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};
} // namespace Lumen
