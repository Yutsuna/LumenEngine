/**
 * @file RHIRingAllocator.hpp
 * @brief Declares a circular range allocator.
 */
#pragma once

#include <HAL/PlatformMutex.hpp>
#include <RHI/Memory/IRHIAllocator.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
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
        /**
         * @brief Reserves an aligned range.
         * @param InDesc Size, alignment and memory class of the request.
         * @return The placed allocation, or InvalidArgument / OutOfMemory with the Memory condition.
         */
        TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;
        /**
         * @brief Returns a range to the allocator.
         * @param InAllocation Allocation previously returned by Allocate.
         * @return Success, or InvalidArgument for a foreign, invalid or double free.
         */
        TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept override;
        /**
         * @brief Reclaims all ranges; callers must ensure no GPU work uses them.
         */
        void Reset () noexcept override;
        /**
         * @brief Returns a coherent snapshot of allocation statistics.
         * @return Current capacity, usage and counts.
         */
        FRHIAllocatorStats GetStats () const noexcept override;

    private:

        UInt64 Capacity;
        UInt64 Cursor = 0ULL;
        UInt64 NextId = 1ULL;
        TVector<FRHIAllocation> Active;
        FRHIAllocatorStats Stats;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };
} // namespace RHI

} // namespace Lumen
