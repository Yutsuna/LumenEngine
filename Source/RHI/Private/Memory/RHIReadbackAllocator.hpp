/**
 * @file RHIReadbackAllocator.hpp
 * @brief Declares the mapped readback ring implementation.
 */
#pragma once
#include "../Types/RHIObjectForward.hpp"
#include <Memory/IRHIReadbackAllocator.hpp>
#include <Memory/RHIRingAllocator.hpp>
#include <Memory/UniquePtr.hpp>
namespace Lumen
{

/**
 * @class FRHIReadbackAllocator
 * @brief Ring allocator backed by readable CPU storage.
 */
class RHI_EXPORT FRHIReadbackAllocator final : public IRHIReadbackAllocator
{
public:

    LUMEN_RHI_OBJECT_DECLARATIONS;
    /** @brief Creates storage and a ring of InCapacity bytes. */
    FRHIReadbackAllocator ( UInt64 InCapacity, FRHIObjectId InId ) noexcept;
    TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;
    TExpected<FRHIReadbackAllocation, FRHIError> Map ( const FRHIAllocation &InAllocation ) noexcept override;
    TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept override;

private:

    FRHIObjectBase Identity;
    FRHIRingAllocator Ring;
    TUniquePtr<Byte[]> Storage;
    USize Capacity = 0U;
};
} // namespace Lumen
