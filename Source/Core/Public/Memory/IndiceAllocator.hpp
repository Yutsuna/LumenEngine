/**
 * @file IndiceAllocator.hpp
 * @brief Declare FIndiceAllocator and FIndiceHeapAllocator, allocators for indices.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <CoreExport.hpp>

#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

#include <HAL/PlatformMutex.hpp>

namespace Lumen
{

/**
 * @struct FIndiceAllocatorRange
 * @brief Represents a range of indices in the indice allocator.
 */
struct FIndiceAllocatorRange
{
    LUMEN_FORCE_INLINE FIndiceAllocatorRange ( UInt32 InFirst, UInt32 InLast ) noexcept;

    UInt32 First;
    UInt32 Last;
};

/**
 * @struct FIndiceAllocation
 * @brief Represents an allocation of contiguous indices.
 */
struct FIndiceAllocation
{
    LUMEN_FORCE_INLINE FIndiceAllocation ( UInt32 InStart, UInt32 InCount ) noexcept;

    UInt32 Start;
    UInt32 Count;
};

/**
 * @class FIndiceAllocator
 * @brief Free-list allocator managing contiguous ranges of variable size.
 *
 * Best suited for variable-length allocations (descriptor tables, dynamic offsets).
 *
 * Thread-safe: allocation, release and allocated-range queries are synchronized.
 * Capacity is immutable after construction.
 */
class CORE_EXPORT FIndiceAllocator
{
public:

    FIndiceAllocator () noexcept  = default;
    ~FIndiceAllocator () noexcept = default;

    /**
     * @brief Constructs an FIndiceAllocator with the specified number of descriptors.
     * @param InNumDescriptors The total capacity of the allocator.
     */
    LUMEN_FORCE_INLINE explicit FIndiceAllocator( UInt32 InNumDescriptors ) noexcept;

public:

    /**
     * @brief Allocates a contiguous range of indices using first-fit.
     * @param InNumDescriptors The number of indices to allocate.
     * @return An optional allocation if successful.
     */
    TOptional<FIndiceAllocation> Allocate ( UInt32 InNumDescriptors ) noexcept;

    /**
     * @brief Frees a previously allocated contiguous range.
     * @note The allocation must have been returned by Allocate() and must not have been freed already.
     */
    void Free ( FIndiceAllocation InAllocation ) noexcept;

    /**
     * @brief Computes the smallest range enclosing all currently allocated indices.
     * @return The enclosing range, or an empty Optional if no indices are allocated.
     */
    [[nodiscard]] TOptional<FIndiceAllocatorRange> TryGetAllocatedRange () const noexcept;

    /**
     * @return The total capacity of the allocator.
     * @note Capacity is immutable and this operation does not require locking.
     */
    [[nodiscard]] LUMEN_FORCE_INLINE UInt32 GetCapacity () const noexcept;

private:

    TVector<FIndiceAllocatorRange> Ranges; /**< Free ranges, sorted and non-overlapping. */

    UInt32 Capacity = 0U;

    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};

/**
 * @class FIndiceHeapAllocator
 * @brief Bitmap-backed allocator over a fixed-capacity index heap.
 *
 * Best suited for fixed-size and contiguous index allocations.
 *
 * Thread-safe: allocation, release and allocated-count queries are synchronized.
 * Capacity is immutable after construction.
 */
class CORE_EXPORT FIndiceHeapAllocator : public FNonCopyable
{
public:

    FIndiceHeapAllocator () noexcept  = default;
    ~FIndiceHeapAllocator () noexcept = default;

    /**
     * @brief Constructs an FIndiceHeapAllocator with the specified capacity.
     * @param InCapacity The number of indices in the heap.
     */
    LUMEN_FORCE_INLINE explicit FIndiceHeapAllocator( UInt32 InCapacity ) noexcept;

public:

    /**
     * @brief Allocates a contiguous run of indices.
     * @param InNumIndices The number of indices to allocate.
     * @return An optional allocation if successful.
     */
    TOptional<FIndiceAllocation> Allocate ( UInt32 InNumIndices ) noexcept;

    /**
     * @brief Frees a previously allocated contiguous run.
     * @note The allocation must have been returned by Allocate() and must not have been freed already.
     */
    void Free ( FIndiceAllocation InAllocation ) noexcept;

    /**
     * @return The total capacity of the heap.
     * @note Capacity is immutable and this operation does not require locking.
     */
    [[nodiscard]] LUMEN_FORCE_INLINE UInt32 GetCapacity () const noexcept;

    /**
     * @return The number of currently allocated indices.
     * @note Thread-safe.
     */
    [[nodiscard]] UInt32 GetAllocatedCount () const noexcept;

public:

    static constexpr UInt32 BitsPerWord = 64U;

private:

    TVector<UInt64> Bitmap; /**< 1 bit per index: 1 = allocated, 0 = free. */

    /**
     * @brief Index of the first word that still contains at least one free bit.
     * @details Monotonically advances during allocation and moves backward during Free().
     *          NumWords indicates that the heap is completely allocated.
     */
    UInt32 FirstHint = 0U;

    UInt32 Capacity  = 0U;
    UInt32 Allocated = 0U;

    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};

} // namespace Lumen

#include "Inline/IndiceAllocator.inl"
