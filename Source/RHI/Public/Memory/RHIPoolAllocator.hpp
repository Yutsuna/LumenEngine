/**
 * @file RHIPoolAllocator.hpp
 * @brief Declares a fixed-size-class allocator.
 */
#pragma once
#include <HAL/PlatformMutex.hpp>
#include <Memory/IRHIAllocator.hpp>
#include <Memory/IndiceAllocator.hpp>
#include <Memory/UniquePtr.hpp>
#include <RHIExport.hpp>
namespace Lumen
{

/**
 * @class FRHIPoolAllocator
 * @brief Allocates equal-sized slots from a fixed pool.
 */
class RHI_EXPORT FRHIPoolAllocator final : public IRHIAllocator
{
public:

    /** @brief Creates InSlotCount slots of InSlotSize bytes. */
    FRHIPoolAllocator ( UInt64 InSlotSize, UInt32 InSlotCount ) noexcept;
    TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;
    TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept override;
    void Reset () noexcept override;
    FRHIAllocatorStats GetStats () const noexcept override;

private:

    UInt64 SlotSize;
    TUniquePtr<FIndiceHeapAllocator> Slots;
    UInt64 NextId = 1ULL;
    FRHIAllocatorStats Stats;
    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};
} // namespace Lumen
