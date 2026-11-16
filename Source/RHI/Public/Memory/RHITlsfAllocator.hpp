/**
 * @file RHITlsfAllocator.hpp
 * @brief Declares a two-level segregated free-list allocator.
 */
#pragma once
#include <HAL/PlatformMutex.hpp>
#include <Memory/IRHIAllocator.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>
namespace Lumen
{

/**
 * @class FRHITlsfAllocator
 * @brief Variable ranges with size-class lookup and coalescing.
 */
class RHI_EXPORT FRHITlsfAllocator final : public IRHIAllocator
{
public:

    /** @brief Creates an allocator over InCapacity bytes. */
    explicit FRHITlsfAllocator ( UInt64 InCapacity ) noexcept;
    TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;
    TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept override;
    void Reset () noexcept override;
    FRHIAllocatorStats GetStats () const noexcept override;

private:

    struct FFreeRange
    {
        UInt64 Offset;
        UInt64 Size;
    };
    static constexpr USize FirstLevelCount  = 64U;
    static constexpr USize SecondLevelCount = 16U;
    /** @return Bucket indices for a size. */
    static void GetBucket ( UInt64 InSize, USize &OutFirst, USize &OutSecond ) noexcept;
    /** @brief Inserts a range in its size-class list. */
    void InsertFreeRange ( FFreeRange InRange ) noexcept;
    UInt64 Capacity;
    UInt64 NextId = 1ULL;
    TVector<TVector<FFreeRange>> Buckets;
    TVector<FRHIAllocationId> ActiveAllocations;
    FRHIAllocatorStats Stats;
    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};
} // namespace Lumen
