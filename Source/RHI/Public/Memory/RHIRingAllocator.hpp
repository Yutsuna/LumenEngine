/**
 * @file RHIRingAllocator.hpp
 * @brief Declares a circular range allocator.
 */
#pragma once
#include <HAL/PlatformMutex.hpp>
#include <Memory/IRHIAllocator.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>
namespace Lumen
{

/**
 * @class FRHIRingAllocator
 * @brief Circular allocation with explicit release of completed ranges.
 */
class RHI_EXPORT FRHIRingAllocator final : public IRHIAllocator
{
public:

    /** @brief Creates a ring over InCapacity bytes. */
    explicit FRHIRingAllocator ( UInt64 InCapacity ) noexcept;
    TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;
    TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept override;
    void Reset () noexcept override;
    FRHIAllocatorStats GetStats () const noexcept override;

private:

    UInt64 Capacity;
    UInt64 Cursor = 0ULL;
    UInt64 NextId = 1ULL;
    TVector<FRHIAllocation> Active;
    FRHIAllocatorStats Stats;
    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};
} // namespace Lumen
