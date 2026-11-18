/**
 * @file RHILinearAllocator.hpp
 * @brief Declares a bounded bump allocator.
 */
#pragma once

#include <HAL/PlatformMutex.hpp>
#include <RHI/Memory/IRHIAllocator.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class FRHILinearAllocator
     * @brief Constant-time allocation with global reset.
     * @details Hands out increasing offsets from a fixed-capacity region. Individual
     *          allocations are not reclaimed; Free only validates the range and
     *          storage is reclaimed together by Reset. Allocate, Reset and GetStats
     *          are serialized by an internal recursive shared mutex, so the allocator
     *          is safe for concurrent clients without external locking. No capability
     *          gate applies to a linear allocator.
     */
    class RHI_EXPORT FRHILinearAllocator final : public IRHIAllocator
    {
    public:

        /**
         * @brief Creates an allocator over InCapacity bytes.
         * @param InCapacity Total number of bytes available before exhaustion.
         */
        explicit FRHILinearAllocator ( UInt64 InCapacity ) noexcept;

        /**
         * @brief Reserves the next available aligned range.
         * @param InDesc Size, alignment and memory class of the requested range.
         * @return The placed allocation on success; InvalidArgument if InDesc fails
         *         validation, or OutOfMemory with the Memory condition on alignment
         *         overflow or when the region is exhausted.
         */
        TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;

        /**
         * @brief Rejects an individual free; storage is reclaimed only by Reset.
         * @param InAllocation Allocation previously returned by Allocate.
         * @return Success for a valid allocation; InvalidArgument with the Memory
         *         condition when InAllocation is invalid. No storage is released.
         */
        TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept override;

        /** @brief Reclaims the whole region; safe to call concurrently with other members. */
        void Reset () noexcept override;

        /**
         * @brief Returns a coherent snapshot of the allocation statistics.
         * @return Current capacity, usage and counts, gathered under the internal lock.
         */
        FRHIAllocatorStats GetStats () const noexcept override;

    private:

        UInt64 Capacity;
        UInt64 Current = 0ULL;
        UInt64 NextId  = 1ULL;
        FRHIAllocatorStats Stats;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };
} // namespace RHI

} // namespace Lumen
