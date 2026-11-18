/**
 * @file RHIUploadAllocator.hpp
 * @brief Declares the mapped upload ring implementation.
 */
#pragma once

#include "../Types/RHIObjectForward.hpp"
#include <Memory/UniquePtr.hpp>
#include <RHI/Memory/IRHIUploadAllocator.hpp>
#include <RHI/Memory/RHIRingAllocator.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class FRHIUploadAllocator
     * @brief Ring allocator backed by writable CPU storage.
     */
    class RHI_EXPORT FRHIUploadAllocator final : public IRHIUploadAllocator
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /** @brief Creates storage and a ring of InCapacity bytes. */
        FRHIUploadAllocator ( UInt64 InCapacity, FRHIObjectId InId ) noexcept;

        TExpected<FRHIUploadAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;

        TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept override;

    private:

        FRHIObjectBase Identity;
        FRHIRingAllocator Ring;
        TUniquePtr<Byte[]> Storage;

        USize Capacity = 0U;
    };

} // namespace RHI

} // namespace Lumen
