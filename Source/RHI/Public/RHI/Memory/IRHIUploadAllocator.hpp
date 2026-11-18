/**
 * @file IRHIUploadAllocator.hpp
 * @brief Declares staging upload allocation.
 */

#pragma once

#include <Memory/Span.hpp>
#include <RHI/Memory/IRHIAllocator.hpp>
#include <RHI/Types/IRHIObject.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHIUploadAllocation
     * @brief Writable staging range and its allocation metadata.
     */
    struct FRHIUploadAllocation
    {
        FRHIAllocation Allocation;
        TSpan<Byte> MappedData;
    };

    /**
     * @class IRHIUploadAllocator
     * @brief CPU-writable ring for later GPU copies.
     */
    class IRHIUploadAllocator : public IRHIObject
    {
    public:

        /** @brief Allocates and maps a staging range. */
        [[nodiscard]] virtual TExpected<FRHIUploadAllocation, FRHIError>
        Allocate ( const FRHIAllocationDesc &InDesc ) noexcept = 0;
        /** @brief Releases a completed staging range. */
        [[nodiscard]] virtual TExpected<void, FRHIError> Free ( const FRHIAllocation &InAllocation ) noexcept = 0;
    };
} // namespace RHI

} // namespace Lumen
