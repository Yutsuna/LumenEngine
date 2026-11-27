/**
 * @file RHIPoolAllocator.hpp
 * @brief Declares a fixed-size-class allocator.
 */

#pragma once

#include <HAL/PlatformMutex.hpp>

#include <Memory/IndiceAllocator.hpp>
#include <Memory/UniquePtr.hpp>
#include <RHI/Memory/IRHIAllocator.hpp>

#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class FRHIPoolAllocator
     * @brief Allocates equal-sized slots from a fixed pool.
     */
    class RHI_EXPORT FRHIPoolAllocator final : public IRHIAllocator
    {
    public:

        /**
         * @brief Creates InSlotCount slots of InSlotSize bytes.
         * @details A zero or non-power-of-two InSlotSize, or a capacity that overflows UInt64,
         *          creates a disabled pool with zero capacity whose allocations all fail.
         */
        FRHIPoolAllocator ( UInt64 InSlotSize, UInt32 InSlotCount ) noexcept;

        /**
         * @brief Reserves an aligned range.
         * @param InDesc Size, alignment and memory class of the request.
         * @return The placed allocation, or InvalidState / Unsupported / OutOfMemory with the Memory condition.
         */
        TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;

        /**
         * @brief Returns a range to the allocator.
         * @param InAllocation Allocation previously returned by Allocate.
         * @return Success, or InvalidArgument for a foreign, invalid or double free.
         */
        FRHIExpected Free ( const FRHIAllocation &InAllocation ) noexcept override;

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

        UInt64 SlotSize = { 0 };

        TUniquePtr<FIndiceHeapAllocator> Slots;
        TVector<FRHIAllocationId> ActiveAllocations;

        UInt64 NextId = 1ULL;
        FRHIAllocatorStats Stats;

        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };
} // namespace RHI

} // namespace Lumen
