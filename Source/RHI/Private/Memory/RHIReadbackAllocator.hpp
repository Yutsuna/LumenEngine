/**
 * @file RHIReadbackAllocator.hpp
 * @brief Declares the mapped readback ring implementation.
 */
#pragma once

#include <RHI/Memory/IRHIReadbackAllocator.hpp>
#include <RHI/Memory/RHIRingAllocator.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Types/RHIObjectForward.hpp>
#include <Templates/Map.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class FRHIReadbackAllocator
     * @brief Ring allocator backed by a readable readback buffer.
     */
    class RHI_EXPORT FRHIReadbackAllocator final : public IRHIReadbackAllocator
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /** @brief Creates a ring over the complete readback buffer. */
        FRHIReadbackAllocator ( TSharedRef<IRHIBuffer> InBuffer, FRHIObjectId InId ) noexcept;

        /** @brief Allocates a contiguous region of the readback buffer. */
        TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept override;

        /** @brief Maps an allocated region of the readback buffer for reading. */
        TExpected<FRHIReadbackAllocation, FRHIError> Map ( const FRHIAllocation &InAllocation ) noexcept override;

        /** @brief Returns the underlying buffer for the readback allocator. */
        TExpected<TSharedRef<IRHIBuffer>, FRHIError> GetBuffer ( const FRHIAllocation &InAllocation ) noexcept override;

        /** @brief Frees an allocated region of the readback buffer. */
        FRHIExpected Free ( const FRHIAllocation &InAllocation ) noexcept override;

    private:

        [[nodiscard]] FRHIExpected ValidateAllocation ( const FRHIAllocation &InAllocation ) const noexcept;

    private:

        FRHIObjectBase Identity;
        TSharedRef<IRHIBuffer> Buffer;
        FRHIRingAllocator Ring;
        USize Capacity = 0U;

        /** @brief Ids issued by this allocator; foreign or released ids must not be mappable. */
        TMap<UInt64, FRHIAllocationId> IssuedIds;
    };
} // namespace RHI

} // namespace Lumen
