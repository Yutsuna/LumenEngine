/**
 * @file RHIUploadAllocator.hpp
 * @brief Declares the mapped upload ring implementation.
 */
#pragma once

#include <RHI/Memory/IRHIUploadAllocator.hpp>
#include <RHI/Memory/RHIRingAllocator.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

namespace Lumen
{

namespace RHI
{
    /**
     * @class FRHIUploadAllocator
     * @brief Ring allocator backed by a writable upload buffer.
     */
    class RHI_EXPORT FRHIUploadAllocator final : public IRHIUploadAllocator
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /** @brief Creates a ring over the complete upload buffer. */
        FRHIUploadAllocator ( TSharedRef<IRHIBuffer> InBuffer, FRHIObjectId InId ) noexcept;

        /** @brief Allocates a contiguous region of the upload buffer. */
        TExpected<FRHIUploadAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;

        /** @brief Frees an allocated region of the upload buffer. */
        FRHIExpected Free ( const FRHIAllocation &InAllocation ) noexcept override;

    private:

        FRHIObjectBase Identity;
        TSharedRef<IRHIBuffer> Buffer;
        FRHIRingAllocator Ring;
        USize Capacity = 0U;
    };
} // namespace RHI

} // namespace Lumen
